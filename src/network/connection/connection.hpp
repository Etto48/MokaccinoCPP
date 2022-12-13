#pragma once
#include <string>
#include <boost/asio/ip/udp.hpp>
namespace network::connection
{
    /**
     * @brief struct that stores info about the pending connection with another peer
     * 
     */
    struct StatusEntry
    {
        std::string expected_message;
        std::string name;
        boost::posix_time::ptime registration_time;
    };
    /**
     * @brief used to access information about the pending connections with other peers stored in status_map
     * 
     */
    extern std::mutex status_map_mutex;
    /**
     * @brief info about pending connections with other peers
     * 
     */
    extern std::map<boost::asio::ip::udp::endpoint,StatusEntry> status_map;
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
     * @param whitelist list of users to automatically accept
     * @param default_action what to do if a user is not in the whitelist
     * 
     */
    void init(std::string local_username, const std::vector<std::string>& whitelist, const std::string& default_action);
    /**
     * @brief request the connection to a specific endpoint, if you are connecting
     * to a server it must have the same username as the hostname
     * 
     * @param endpoint the target endpoint
     * @param expected_name the name expected from the other endpoint
     */
    void connect(const boost::asio::ip::udp::endpoint& endpoint,const std::string& expected_name);
    /**
     * @brief this must be used to notify the connection module that a pending connection timed out
     * 
     * @param endpoint the udp endpoint of the user that timed out
     * @return true if the user was found
     * @return false if the user was not found
     */
    bool connection_timed_out(const boost::asio::ip::udp::endpoint& endpoint);
}