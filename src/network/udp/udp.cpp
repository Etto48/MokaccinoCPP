#include "udp.hpp"
#include "../../defines.hpp"
#include "../../ansi_escape.hpp"
#include <boost/asio/io_service.hpp>
#include <boost/date_time.hpp>
#include <boost/thread.hpp>
#include "../../multithreading/multithreading.hpp"
#include "../../logging/logging.hpp"
#include "../../parsing/parsing.hpp"
#include "crypto/crypto.hpp"
namespace network::udp
{
    DataMap connection_map;

    auto io_service = boost::asio::io_service{};
    boost::asio::ip::udp::socket socket(io_service);
    boost::asio::ip::udp::endpoint local_endpoint;

    std::mutex message_queue_association_mutex;
    struct QueueAssociationEntry
    {
        MessageQueue* queue;
        bool connection_required;
    };
    std::map<std::string,QueueAssociationEntry> message_queue_association;

    std::mutex requested_clients_mutex;
    std::map<std::string,boost::posix_time::ptime> requested_clients;

    bool send(std::string message, const std::string& name)
    {
        try{
            std::unique_lock lock(udp::connection_map.obj);
            auto& info = connection_map[name];
            auto endpoint = info.endpoint;
            if(info.encrypted)
            {
                auto enc = crypto::encrypt(message,info.symmetric_key);
                message = parsing::compose_message({"C",std::get<0>(enc),std::get<1>(enc),std::get<2>(enc)});
            }
            message+='\n';    
            socket.send_to(boost::asio::buffer(message,message.length()),endpoint);
            return true;
        }catch(DataMap::NotFound&){
            return false;
        }
        
    }
    void send(std::string message, const boost::asio::ip::udp::endpoint& endpoint)
    {
        try
        {
            try
            {
                std::unique_lock lock(connection_map.obj);
                auto& info = connection_map[endpoint];
                if(info.encrypted)
                {
                    auto enc = crypto::encrypt(message,info.symmetric_key);
                    message = parsing::compose_message({"C",std::get<0>(enc),std::get<1>(enc),std::get<2>(enc)});
                }
            }
            catch(DataMap::NotFound&)
            {}
            message+='\n';       
            socket.send_to(boost::asio::buffer(message,message.length()),endpoint);
        }catch(boost::system::system_error& e)
        {
            logging::log("ERR",e.what());
        }
        
    }
    void handle_message(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint, std::string msg)
    {
        msg.pop_back();//remove '\n'
        
        /*
        if(name.length() == 0)
            logging::message_log(endpoint.address().to_string() + ":" + std::to_string(endpoint.port()),msg);
        else
            logging::message_log(name,msg);*/
        
        auto keyword = parsing::get_msg_keyword(msg);
        if(keyword == "C")
        {
            if(name.length() != 0)
            {
                try{
                    std::unique_lock lock(connection_map.obj);
                    auto& info = connection_map[name];
                    if(info.encrypted)
                    {
                        auto args = parsing::msg_split(msg);
                        if(args.size() == 4)
                        {
                            msg = crypto::decrypt(args[1],info.symmetric_key,args[2],args[3]);
                            keyword = parsing::get_msg_keyword(msg);
                        }
                        else
                            return;
                    }
                }catch(DataMap::NotFound&)
                {}
            }
            else
            {
                logging::log("DBG","Dropped encrypted message from anonymous user (" HIGHLIGHT + endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET ")");
                return;
            }
        }

        std::unique_lock lock(message_queue_association_mutex);
        auto queue = message_queue_association.find(keyword);
        if(queue!=message_queue_association.end())
        {
            if(name != "" or not queue->second.connection_required)
            {
                queue->second.queue->push({name,endpoint,msg});
            }else
            {
                logging::log("DBG","Message \"" HIGHLIGHT + keyword + RESET "\" refused from anonymous user (" HIGHLIGHT + endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET ")");
            }
        }
        else
        {
            logging::log("DBG","Message keyword \"" HIGHLIGHT +keyword+ RESET "\" not recognized");
        }
    }
    void udp_listener()
    {
        while(true)
        {
            auto len = socket.available();
            if(len==0)
            {
                boost::this_thread::interruption_point();
                boost::this_thread::yield(); // not wasting time now
                continue;
            }
            auto data = new char[len+1];
            boost::asio::ip::udp::endpoint sender_endpoint;
            try{
                auto recv_len = socket.receive_from(boost::asio::buffer(data,len),sender_endpoint);
                assert(recv_len<=len);
                data[recv_len] = '\0';
            }catch(boost::system::system_error&)
            { // someone tried to connect to itself... ingnore it
                delete[] data;
                continue;
            }
            
            std::string recv_data = data;
            delete[] data;
            try
            {
                std::unique_lock lock(udp::connection_map.obj);
                auto& peerdata = connection_map[sender_endpoint];
                peerdata.tmpData+=recv_data;
                if(peerdata.tmpData.back()=='\n')
                {
                    handle_message(peerdata.name,sender_endpoint,peerdata.tmpData);
                    peerdata.tmpData="";
                }
            }catch(DataMap::NotFound&)
            {
                handle_message("",sender_endpoint,recv_data);
            }
        }
    }
    void init(uint16_t port)
    {
        local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("0.0.0.0"),port);
        socket.open(IP_VERSION);
        socket.bind(local_endpoint);

        multithreading::add_service("udp_listener",udp_listener); 
        auto localhost = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"),port);

        if(DEBUG)
            connection_map.add_user("loopback",localhost);
    }
    void register_queue(std::string keyword, MessageQueue& queue, bool connection_required)
    {
        std::unique_lock lock(message_queue_association_mutex);
        message_queue_association[keyword] = {&queue,connection_required};
    }
    const char* LookupError::what()
    {
        return "Error resolving DNS query";
    }
    boost::asio::ip::udp::endpoint dns_lookup(const std::string& hostname, uint16_t port)
    {
        boost::asio::ip::udp::resolver::query query{hostname,std::to_string(port)};
        boost::asio::ip::udp::resolver resolver{io_service};
        boost::system::error_code ec;
        auto it = resolver.resolve(query,ec);
        if(ec)
        {
            throw LookupError{};
        }
        else
        {   
            for(auto& endpoint: it)
            {
                if(endpoint.endpoint().protocol() == IP_VERSION)
                {
                    return endpoint;
                }
            }
            throw LookupError{};
        }
    }
    void server_request(const std::string& name)
    {
        {
            std::unique_lock lock(requested_clients_mutex);
            requested_clients[name] = boost::posix_time::microsec_clock::local_time();
        }
        send(parsing::compose_message({"REQUEST",name}),connection_map.server());
    }
    bool server_request_success(const std::string& name)
    {
        bool requested = false;
        {
            std::unique_lock lock(requested_clients_mutex);
            if(requested_clients.find(name) != requested_clients.end())
            {
                requested = true;
                requested_clients.erase(name);
            }
        }
        if(requested)
        {
            logging::log("MSG","User \"" HIGHLIGHT +name+ RESET "\" found");
            return true;
        }
        return false;
    }
    void server_request_fail(const std::string& name)
    {
        {
            std::unique_lock lock(requested_clients_mutex);
            requested_clients.erase(name);
        }
    }
}