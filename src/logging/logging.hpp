#pragma once
#include "../defines.hpp"
#include <iostream>
#include <string>
#include <mutex>
#include <stdint.h>
#include <boost/asio/ip/udp.hpp>
#include "../network/MessageQueue/MessageQueue.hpp"
#include "../terminal/prompt.hpp"
namespace logging
{
    void log(std::string log_type,std::string message);
    
    void message_log(std::string src, std::string message);
    void new_thread_log(std::string thread_name);
    void supervisor_log(size_t connections, size_t services);
    void new_user_log(std::string name, const boost::asio::ip::udp::endpoint& endpoint);
    void removed_user_log(std::string name);
    void connection_test_log(const network::MessageQueueItem& item);
    void terminal_processing_log(const std::string& line);

    void sent_text_message_log(const std::string& to, const std::string& msg);
    void recieved_text_message_log(const std::string& from, const std::string& msg);

    void user_not_found_log(const std::string& name);
    void lookup_error_log(const std::string& host);
    void no_server_available_log();
    void command_not_found_log(const std::string& line);
}