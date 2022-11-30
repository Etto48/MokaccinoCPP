#include "DataMap.hpp"
namespace network
{
    bool DataMap::_add_user(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint)
    {
        if(name_data.find(name)!=name_data.end())//found
            return false;
        else if(endpoint_name.find(endpoint)!=endpoint_name.end())//found
            return false;
        else
        {
            if(name != "loopback")
                server_name = name;
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
    DataMap::PeerData& DataMap::_op_sqbr(const std::string& name)
    {
        auto ret = name_data.find(name);
        if(ret==name_data.end())
            throw NotFound{};
        return (*ret).second;
    }
    DataMap::PeerData& DataMap::_op_sqbr(const boost::asio::ip::udp::endpoint& endpoint)
    {
        auto ret = endpoint_name.find(endpoint);
        if(ret==endpoint_name.end())
            throw NotFound{};
        return name_data[(*ret).second];
    }

    bool DataMap::add_user(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint)
    {
        std::unique_lock lock(obj);
        logging::new_user_log(name,endpoint);
        return _add_user(name,endpoint);
    }
    bool DataMap::remove_user(const std::string& name)
    {
        std::unique_lock lock(obj);
        logging::removed_user_log(name);
        return _remove_user(name);
    }
    bool DataMap::remove_user(const boost::asio::ip::udp::endpoint& endpoint)
    {
        std::unique_lock lock(obj);
        return _remove_user(endpoint);
    }
    size_t DataMap::size()
    {
        std::unique_lock lock(obj);
        return name_data.size();
    }
    DataMap::PeerData& DataMap::operator[](const std::string& name)
    {
        std::unique_lock lock(obj);
        return _op_sqbr(name);
    }
    DataMap::PeerData& DataMap::operator[](const boost::asio::ip::udp::endpoint& endpoint)
    {
        std::unique_lock lock(obj);
        return _op_sqbr(endpoint);
    }
    bool DataMap::check_user(const std::string& name)
    {
        std::unique_lock lock(obj);
        return name_data.find(name) != name_data.end();
    }
    bool DataMap::check_user(const boost::asio::ip::udp::endpoint& endpoint)
    {
        std::unique_lock lock(obj);
        return endpoint_name.find(endpoint) != endpoint_name.end();
    }
    boost::asio::ip::udp::endpoint DataMap::server()
    {
        std::unique_lock lock(obj);
        return _op_sqbr(server_name).endpoint;
    }
}