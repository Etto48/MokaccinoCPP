#include "udp.hpp"
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

    void init(uint16_t port)
    {
        local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("0.0.0.0"),port);
        socket.open(IP_VERSION);
        socket.bind(local_endpoint);

        multithreading::add_service("network_udp_listener",listener); 
        auto localhost = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"),port);

        #ifdef _DEBUG
        connection_map.add_user("loopback",localhost);
        #endif
    }

    bool send(std::string message, const std::string& name)
    {
        try{
            message+='\n';
            auto endpoint = connection_map[name].endpoint;
            socket.send_to(boost::asio::buffer(message,message.length()),endpoint);
            return true;
        }catch(DataMap::NotFound&){
            return false;
        }
        
    }
    void send(std::string message, const boost::asio::ip::udp::endpoint& endpoint)
    {
        message+='\n';
        try
        {
            socket.send_to(boost::asio::buffer(message,message.length()),endpoint);
        }catch(boost::system::system_error& e)
        {
            logging::log("ERR",e.what());
        }
        
    }
    void handle_message(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint, std::string msg)
    {
        msg.pop_back();//remove '\n'
        logging::message_log(name,msg);
        
        auto keyword = parsing::get_msg_keyword(msg);

        std::unique_lock lock(message_queue_association_mutex);
        auto queue = message_queue_association.find(keyword);
        if(queue!=message_queue_association.end())
        {
            if(name != "" || !queue->second.connection_required)
            {
                queue->second.queue->push({name,endpoint,msg});
            }else
            {
                logging::log("DBG","Message refused from anonymous user");
            }
        }
        else
        {
            logging::log("DBG",_format("Message keyword \"{}\" not recognized",keyword));
        }
    }
    void listener()
    {
        while(true)
        {
            auto len = socket.available();
            if(len==0)
            {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
                continue;
            }
            auto data = new char[len+1];
            boost::asio::ip::udp::endpoint sender_endpoint;
            auto recv_len = socket.receive_from(boost::asio::buffer(data,len),sender_endpoint);
            assert(recv_len<=len);
            data[recv_len] = '\0';
            std::string recv_data = data;
            delete[] data;
            try
            {
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
            requested_clients[name] = boost::posix_time::second_clock::local_time();
        }
        send(parsing::compose_message({"REQUEST",name}),connection_map.server());
    }
}