#pragma once
#include "../defines.hpp"
#include <string>
#include <map>
#include <mutex>
#include <condition_variable>
#include <boost/thread/thread.hpp>
#include <boost/asio/signal_set.hpp>
#include "../logging/logging.hpp"
namespace multithreading
{
    void add_service(std::string name,void (*f)());
    void join();
    void stop(std::string name);
    void stop_all();
    size_t get_count();
    void request_termination();
    void wait_termination();
}