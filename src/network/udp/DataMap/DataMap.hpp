#pragma once
#include "../../../defines.hpp"
#include <map>
#include <mutex>
#include <string>
#include <boost/asio/ip/udp.hpp>
#include "../../../logging/logging.hpp"

namespace network
{
    class DataMap
    {
    public:
        struct PeerData
        {
            std::string name;
            boost::asio::ip::udp::endpoint endpoint;
            std::string tmpData;
        };
    private:
        std::map<boost::asio::ip::udp::endpoint,std::string> endpoint_name;
        std::map<std::string,PeerData> name_data;    
        std::mutex obj;
        std::string server_name;
    public:
        
        bool add_user(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint);
        bool remove_user(const std::string& name);
        bool remove_user(const boost::asio::ip::udp::endpoint& endpoint);

        size_t size();
        class NotFound: public std::exception
        {
        public:
            const char* what();
        };

        PeerData& operator[](const std::string& name);
        PeerData& operator[](const boost::asio::ip::udp::endpoint& endpoint);

        bool check_user(const std::string& name);
        bool check_user(const boost::asio::ip::udp::endpoint& endpoint);
        boost::asio::ip::udp::endpoint server();
    private:
        bool _add_user(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint);
        bool _remove_user(const std::string& name);
        bool _remove_user(const boost::asio::ip::udp::endpoint& endpoint);

        PeerData& _op_sqbr(const std::string& name);
        PeerData& _op_sqbr(const boost::asio::ip::udp::endpoint& endpoint);
    };
}