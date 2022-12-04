#pragma once
#include <stdint.h>
#include "DataMap/DataMap.hpp"
#include <string>
#include <exception>
#include <boost/asio/ip/udp.hpp>
#include "../MessageQueue/MessageQueue.hpp"

namespace network::udp
{
    /**
     * @brief contains every connection to another peer (if DEBUG is set it contains even a "loopback" connection)
     * 
     */
    extern DataMap connection_map;
    /**
     * @brief initialize the module
     * 
     * @param port the port on which we will listen (UDP)
     */
    void init(uint16_t port);
    /**
     * @brief send a message to a connected user, the name is searched inside connection_map
     * 
     * @param message the message to send, '\n' will be added automatically at the end of the string
     * @param name the destination
     * @return true if the user was found inside connection_map
     * @return false if the user is not connected
     */
    bool send(std::string message, const std::string& name);
    /**
     * @brief send a message to an endpoint, this don't need to be already connected
     * 
     * @param message the message to send, '\n' will be added automatically at the end of the string
     * @param endpoint the destination
     */
    void send(std::string message, const boost::asio::ip::udp::endpoint& endpoint);
    /**
     * @brief register a MessageQueue linking it to a specific keyword, when a message with that keyword\
     * is recived, it will be added to the queue, you can require the source endpoint to be registered, if you do so
     * any message with the selected keyword will be dropped if the source was no registered
     * 
     * @param keyword the keyword to associate with the queue
     * @param queue the queue where you want your messages to be stored
     * @param connection_required true if you want to exclude messages from an endpoint that is not registered
     */
    void register_queue(std::string keyword, MessageQueue& queue, bool connection_required);
    /**
     * @brief exception thrown by dns_lookup if no valid IP was found
     * 
     */
    class LookupError : public std::exception
    {
    public:
        const char* what();
    };
    /**
     * @brief do a hostname lookup, if it fails then throws LookupError
     * 
     * @param hostname the hostname to lookup
     * @param port the port you want in the returned endpoint
     * @return an endpoint with the ip found during the lookup and the port you provided
     */
    boost::asio::ip::udp::endpoint dns_lookup(const std::string& hostname, uint16_t port);
    /**
     * @brief request to a valid peer to open a connection with the requested user if it's 
     * already connected to the peer, this is non blocking, if the search fails then a message from the peer
     * with this format will be received FAIL <username> where username was the name searched
     * 
     * @param name the peer to query for
     */
    void server_request(const std::string& name);
}