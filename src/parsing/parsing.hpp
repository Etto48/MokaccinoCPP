#pragma once
#include "../defines.hpp"
#include <string>
#include <vector>
#include <boost/asio/ip/udp.hpp>
namespace parsing
{
    std::string get_msg_keyword(const std::string& msg);
    std::vector<std::string> msg_split(const std::string& msg);
    std::vector<std::string> cmd_args(const std::string& line);
    class EndpointFromStrError: public std::exception
    {
    public:
        const char* what();
    };
    boost::asio::ip::udp::endpoint endpoint_from_str(const std::string& str);
    std::string compose_message(const std::vector<std::string>& components);
}