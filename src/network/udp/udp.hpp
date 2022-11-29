#pragma once
#include <stdint.h>
#include <string>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/array.hpp>
#include "../../multithreading/multithreading.hpp"
#include "../../logging/logging.hpp"
#include "DataMap/DataMap.hpp"
#include "MessageQueue/MessageQueue.hpp"
namespace network::udp
{
    extern MessageQueue messagequeue;
    extern DataMap datamap;
    void init(uint16_t port);
    bool send(std::string message, const std::string& name);
    void listener();
}