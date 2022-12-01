#pragma once
#include "../../defines.hpp"
#include <map>
#include "../../multithreading/multithreading.hpp"
#include "../../parsing/parsing.hpp"
#include "../udp/udp.hpp"
#include "../MessageQueue/MessageQueue.hpp"
namespace network::connection
{
    /**
     * @brief the username of this peer
     * 
     */
    extern std::string username;
    /**
     * @brief initialize the module
     * 
     * @param local_username the username to initialize the username variable
     */
    void init(std::string local_username);
    /**
     * @brief request the connection to a specific endpoint, if you are connecting
     * to a server it must have the same username as the hostname
     * 
     * @param endpoint the target endpoint
     * @param expected_name the name expected from the other endpoint
     */
    void connect(const boost::asio::ip::udp::endpoint& endpoint,const std::string& expected_name);
}