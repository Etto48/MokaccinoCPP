#pragma once
#include "../../defines.hpp"
#include <map>
#include "../../multithreading/multithreading.hpp"
#include "../../parsing/parsing.hpp"
#include "../udp/udp.hpp"
#include "../MessageQueue/MessageQueue.hpp"
namespace network::connection
{
    extern std::string username;
    void init(std::string local_username);
    void connect(const boost::asio::ip::udp::endpoint& endpoint,const std::string& expected_name);
}