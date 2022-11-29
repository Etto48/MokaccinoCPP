#include <string>
#include <map>
#include <iostream>
#include <exception>
#include <mutex>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/array.hpp>
#include "../../multithreading/multithreading.hpp"
namespace network::udp
{
    struct PeerData
    {
        std::string name;
        boost::asio::ip::udp::endpoint endpoint;
        std::string tmpData;
    };
    class DataMap
    {
    private:
        std::map<boost::asio::ip::udp::endpoint,std::string> endpoint_name;
        std::map<std::string,PeerData> name_data;    
        std::mutex obj;
    public:
        bool add_user(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint);
        bool remove_user(const std::string& name);
        bool remove_user(const boost::asio::ip::udp::endpoint& endpoint);

        class NotFound: public std::exception
        {
        public:
            const char* what();
        };

        PeerData& operator[](const std::string& name);
        PeerData& operator[](const boost::asio::ip::udp::endpoint& endpoint);
    private:
        bool _add_user(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint);
        bool _remove_user(const std::string& name);
        bool _remove_user(const boost::asio::ip::udp::endpoint& endpoint);

        PeerData& _op_sqbr(const std::string& name);
        PeerData& _op_sqbr(const boost::asio::ip::udp::endpoint& endpoint);
    };
    extern DataMap datamap;
    void init(uint16_t port);
    void listener();
}