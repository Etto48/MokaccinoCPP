#pragma once
#include "../../defines.hpp"
#include <stdint.h>
#include <string>
#include <exception>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/array.hpp>
#include <boost/date_time.hpp>
#include "../../multithreading/multithreading.hpp"
#include "../../logging/logging.hpp"
#include "../../parsing/parsing.hpp"
#include "../MessageQueue/MessageQueue.hpp"
#include "DataMap/DataMap.hpp"
namespace network::udp
{
    extern DataMap connection_map;
    void init(uint16_t port);
    bool send(std::string message, const std::string& name);
    void send(std::string message, const boost::asio::ip::udp::endpoint& endpoint);
    void listener();
    void register_queue(std::string keyword, MessageQueue& queue, bool connection_required);
    class LookupError : public std::exception
    {
    public:
        const char* what();
    };
    boost::asio::ip::udp::endpoint dns_lookup(const std::string& hostname, uint16_t port);
    void server_request(const std::string& name);
}