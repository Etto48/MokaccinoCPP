#pragma once
#include <iostream>
#include <string>
#include <format>
namespace logging
{
    void log(std::string log_type,std::string message);
    
    void message_log(std::string src, std::string message);
    void new_thread_log(std::string thread_name);
}