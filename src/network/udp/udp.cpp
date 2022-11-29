#include "udp.hpp"
namespace network::udp
{
    DataMap datamap;
    MessageQueue messagequeue;

    auto io_service = boost::asio::io_service{};
    boost::asio::ip::udp::socket socket(io_service);
    boost::asio::ip::udp::endpoint local_endpoint;

    void init(uint16_t port)
    {
        local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("0.0.0.0"),port);
        socket.open(boost::asio::ip::udp::v4());
        socket.bind(local_endpoint);

        multithreading::add_service("network_udp_listener",listener); 
        auto localhost = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"),port);

        #ifdef DEBUG
        datamap.add_user("loopback",localhost);
        #endif
    }

    void handle_message(const std::string& name, std::string msg)
    {
        msg.pop_back();//remove '\n'
        logging::message_log(name,msg);

    }
    bool send(std::string message, const std::string& name)
    {
        try{
            message+='\n';
            auto endpoint = datamap[name].endpoint;
            socket.send_to(boost::asio::buffer(message,message.length()),endpoint);
            return true;
        }catch(DataMap::NotFound&){
            return false;
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
                auto& peerdata = datamap[sender_endpoint];
                peerdata.tmpData+=recv_data;
                if(peerdata.tmpData.back()=='\n')
                {
                    handle_message(peerdata.name,peerdata.tmpData);
                    peerdata.tmpData="";
                }
            }catch(DataMap::NotFound&)
            {}
        }
    }
}