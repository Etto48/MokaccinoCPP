#pragma once
#include "../../defines.hpp"
#include <string>
#include <boost/thread/sync_queue.hpp>
#include <boost/asio/ip/udp.hpp>

namespace network
{
    struct MessageQueueItem
    {   
        std::string src;
        boost::asio::ip::udp::endpoint src_endpoint;
        std::string msg;
    };
    typedef boost::sync_queue<MessageQueueItem> MessageQueue;
}