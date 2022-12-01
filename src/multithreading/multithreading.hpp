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
    /**
     * @brief start a new thread
     * 
     * @param name the name of the thread
     * @param f the function to run
     */
    void add_service(std::string name,void (*f)());
    /**
     * @brief wait for all of the threads
     * 
     */
    void join();
    /**
     * @brief stop a specific thread
     * 
     * @param name the thread name
     */
    void stop(std::string name);
    /**
     * @brief stop every thread
     * 
     */
    void stop_all();
    /**
     * @brief get how many threads were initialized with add_service
     * 
     * @return the count
     */
    size_t get_count();
    /**
     * @brief request the termination of the program
     * 
     */
    void request_termination();
    /**
     * @brief used in the main to wait for the termination
     * 
     */
    void wait_termination();
}