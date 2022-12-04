#pragma once
#include <string>

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
    /**
     * @brief get the current thread name as registered in add_service
     * 
     * @return the thread name
     */
    std::string get_current_thread_name();
}