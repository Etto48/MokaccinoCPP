#pragma once
#include <string>
#include <boost/asio/ip/udp.hpp>
namespace network::connection
{
    /**
     * @brief the username of this peer
     * 
     */
    extern std::string username;
    /**
     * @brief a list of names to accept automatically when they request connection
     * 
     */
    extern std::vector<std::string> whitelist;
    /**
     * @brief initialize the module
     * 
     * @param local_username the username to initialize the username variable
     * @param whitelist a list of names to accept automatically when they request connection
     */
    void init(std::string local_username, const std::vector<std::string>& whitelist = {});
    /**
     * @brief request the connection to a specific endpoint, if you are connecting
     * to a server it must have the same username as the hostname
     * 
     * @param endpoint the target endpoint
     * @param expected_name the name expected from the other endpoint
     */
    void connect(const boost::asio::ip::udp::endpoint& endpoint,const std::string& expected_name);
}