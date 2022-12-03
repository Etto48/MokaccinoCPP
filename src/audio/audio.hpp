#pragma once
#include <iostream>
#include <mutex>
#include <boost/thread/sync_bounded_queue.hpp>
#include "../logging/logging.hpp"
#include "../multithreading/multithreading.hpp"
#include "../network/MessageQueue/MessageQueue.hpp"
#include "../network/udp/udp.hpp"
#include "../parsing/parsing.hpp"
namespace audio
{
    /**
     * @brief initialize the module
     * 
     */
    void init();
    /**
     * @brief start a voice call with a connected user if no other call is happening
     * 
     * @param name the user you want to call
     */
    void start_call(const std::string& name);
    /**
     * @brief stop a voice call if there was one
     * 
     */
    void stop_call();
}