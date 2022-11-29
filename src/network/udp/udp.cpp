#include "udp.hpp"
namespace network::udp
{
    DataMap datamap;
    bool DataMap::_add_user(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint)
    {
        if(name_data.find(name)!=name_data.end())//found
            return false;
        else if(endpoint_name.find(endpoint)!=endpoint_name.end())//found
            return false;
        else
        {
            name_data[name] = PeerData{name,endpoint,""};
            endpoint_name[endpoint] = name;
            return true;
        }
    }
    bool DataMap::_remove_user(const std::string& name)
    {
        if(name_data.find(name)==name_data.end())//not found
            return false;
        else
        {
            auto endpoint = name_data[name].endpoint;
            name_data.erase(name);
            endpoint_name.erase(endpoint);
            return true;
        }
    }
    bool DataMap::_remove_user(const boost::asio::ip::udp::endpoint& endpoint)
    {
        if(endpoint_name.find(endpoint)==endpoint_name.end())//not found
            return false;
        else
        {
            auto name = endpoint_name[endpoint];
            endpoint_name.erase(endpoint);
            name_data.erase(name);
            return true;
        }
    }
    const char* DataMap::NotFound::what()
    {
        return "Name/Endpoint not found";
    }
    PeerData& DataMap::_op_sqbr(const std::string& name)
    {
        auto& ret = name_data.find(name);
        if(ret==name_data.end())
            throw NotFound{};
        return (*ret).second;
    }
    PeerData& DataMap::_op_sqbr(const boost::asio::ip::udp::endpoint& endpoint)
    {
        auto& ret = endpoint_name.find(endpoint);
        if(ret==endpoint_name.end())
            throw NotFound{};
        return name_data[(*ret).second];
    }

    bool DataMap::add_user(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint)
    {
        std::unique_lock lock(obj);
        return _add_user(name,endpoint);
    }
    bool DataMap::remove_user(const std::string& name)
    {
        std::unique_lock lock(obj);
        return _remove_user(name);
    }
    bool DataMap::remove_user(const boost::asio::ip::udp::endpoint& endpoint)
    {
        std::unique_lock lock(obj);
        return _remove_user(endpoint);
    }
    PeerData& DataMap::operator[](const std::string& name)
    {
        std::unique_lock lock(obj);
        return _op_sqbr(name);
    }
    PeerData& DataMap::operator[](const boost::asio::ip::udp::endpoint& endpoint)
    {
        std::unique_lock lock(obj);
        return _op_sqbr(endpoint);
    }


    auto service = boost::asio::io_service{};
    boost::asio::ip::udp::socket socket(service);
    boost::asio::ip::udp::endpoint local_endpoint;

    void init(uint16_t port)
    {
        local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("0.0.0.0"),port);
        socket.open(boost::asio::ip::udp::v4());
        socket.bind(local_endpoint);

        multithreading::add_service("network_udp_listener",listener); 
        auto localhost = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"),port);

        datamap.add_user("SELF",localhost);
        std::string data = "PING\n";
        socket.send_to(boost::asio::buffer(data,data.length()),localhost);
    }

    void handle_message(const std::string& name, std::string msg)
    {
        msg.pop_back();//remove '\n'
        #ifdef DEBUG
        std::cout<<"["<<name<<"] "<<msg<<std::endl;
        #endif

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
            delete data;
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