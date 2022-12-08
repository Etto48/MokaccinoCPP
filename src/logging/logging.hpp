#pragma once
#include <string>
#include <mutex>
#include <boost/asio/ip/udp.hpp>
#include "../network/MessageQueue/MessageQueue.hpp"
#include "../toml.hpp"
namespace logging
{
    /**
     * @brief use this if you want to lock the output of the program
     * 
     */
    extern std::mutex output_mutex;
    /**
     * @brief initialize the module
     * 
     * @param log_file if you want to write the log to a file, set this to the path, this will be ignored if set to ""
     */
    void init(const std::string& log_file = "");
    /**
     * @brief print something on the screen
     * 
     * @param log_type one of "ERR" "PRM" "MSG" "DBG"
     * @param message the text to print
     */
    void log(std::string log_type,std::string message);
    
    void message_log(std::string src, std::string message);
    void new_thread_log(std::string thread_name);
    void supervisor_log(size_t connections,size_t services,unsigned long long audio_input_dropped_frames, unsigned long long audio_output_dropped_frames);
    void new_user_log(std::string name, const boost::asio::ip::udp::endpoint& endpoint);
    void removed_user_log(std::string name);
    void connection_test_log(const network::MessageQueueItem& item);
    void terminal_processing_log(const std::string& line);
    void config_success_log(const std::string& path);

    void sent_text_message_log(const std::string& to, const std::string& msg);
    void recieved_text_message_log(const std::string& from, const std::string& msg);
    void received_disconnect_log(const std::string& name, const std::string& reason);
    void sent_disconnect_log(const std::string& name);

    void user_not_found_log(const std::string& name);
    void user_lookup_error_log(const std::string& name);
    void lookup_error_log(const std::string& host);
    void no_server_available_log();
    void command_not_found_log(const std::string& line);
    void config_not_found_log(const std::string& path);
    void config_error_log(const toml::parse_error& e);
    void audio_call_error_log(const std::string& why);
}