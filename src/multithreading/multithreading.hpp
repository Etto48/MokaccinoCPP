#pragma once
#include <string>
#include <map>
#include <boost/thread/thread.hpp>
#include <boost/asio/signal_set.hpp>
#include "../logging/logging.hpp"
namespace multithreading
{
    void add_service(std::string name,void (*f)());
    void join();
    void stop(std::string name);
    void stop_all();
}