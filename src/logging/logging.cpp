#include "logging.hpp"
#ifdef _DEBUG 
    #define VERBOSITY 2
#else
    #define VERBOSITY 1
#endif
namespace logging
{
    #define ERR_TAG "\033[31m"
    #define DBG_TAG "\033[33m"
    #define MSG_TAG "\033[34m"
    #define HIGHLIGHT "\033[35m"
    #define TAG "\033[36m"
    #define RESET "\033[0m"
    std::mutex output_mutex;
    void log(std::string log_type,std::string message)
    {
        std::string terminal_prefix;
        std::string file_prefix;
        unsigned int verbosity_required = 0;
        if(log_type=="ERR")
        {
            file_prefix = "[ERROR] ";
            terminal_prefix = ERR_TAG "[ERROR]" RESET " ";
            verbosity_required = 0;
        }else if(log_type == "MSG")
        {
            file_prefix = "[MESSAGE] ";
            terminal_prefix = MSG_TAG "[MESSAGE]" RESET " ";
            verbosity_required = 1;
        }else if(log_type == "DBG")
        {
            file_prefix = "[DEBUG] ";
            terminal_prefix = DBG_TAG "[DEBUG]" RESET " ";
            verbosity_required = 2;
        }
        if(VERBOSITY >= verbosity_required)
        {
            std::unique_lock lock(output_mutex);
            std::cout << "\r\033[K" << terminal_prefix << message << std::endl;
            terminal::prompt();
        }
    }
    
    void message_log(std::string src, std::string message)
    {
        log("DBG","Message from " HIGHLIGHT + src + RESET " received: \"" HIGHLIGHT + message + RESET "\"");
    }
    void new_thread_log(std::string thread_name)
    {
        log("DBG","Thread " HIGHLIGHT + thread_name + RESET " started");
    }
    void supervisor_log(size_t connections,size_t services)
    {
        log("DBG",TAG "[SUPERVISOR]" RESET " connections:" HIGHLIGHT + std::to_string(connections) + RESET " services:" HIGHLIGHT + std::to_string(services) + RESET);
    }
    void new_user_log(std::string name, const boost::asio::ip::udp::endpoint& endpoint)
    {
        log("DBG","User " HIGHLIGHT + name + RESET ", (" HIGHLIGHT + endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET ") added");
    }
    void removed_user_log(std::string name)
    {
        log("DBG","User " HIGHLIGHT + name + RESET " removed");
    }
    void connection_test_log(const network::MessageQueueItem& item)
    {
        log("DBG",TAG "[CONNECTION]" RESET " Handled test message \"" HIGHLIGHT + item.msg + RESET "\" from " HIGHLIGHT + item.src + RESET "@" HIGHLIGHT + item.src_endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(item.src_endpoint.port()) + RESET);
    }
    void terminal_processing_log(const std::string& line)
    {
        log("DBG",TAG "[TERMINAL]" RESET "Processing command \"" HIGHLIGHT + line + RESET "\"");
    }
    
    #define MESSAGE_PEER "\033[32m"
    #define MESSAGE_TEXT "\033[30;1m"

    void sent_text_message_log(const std::string& to, const std::string& msg)
    {
        log("MSG", MESSAGE_PEER + to + RESET " << " MESSAGE_TEXT + msg + RESET);
    }
    void recieved_text_message_log(const std::string& from, const std::string& msg)
    {
        log("MSG", MESSAGE_PEER + from + RESET " >> " MESSAGE_TEXT + msg + RESET);
    }
    void received_disconnect_log(const std::string& name, const std::string& reason)
    {
        std::string output = "User " HIGHLIGHT+name+RESET " disconnected";
        if(reason.length()>0)
            output += ", reason " HIGHLIGHT+reason+RESET;
        log("MSG", output);
    }
    void sent_disconnect_log(const std::string& name)
    {
        log("MSG", "Disconnected from " HIGHLIGHT + name + RESET);
    }


    void user_not_found_log(const std::string& name)
    {
        log("ERR","User " HIGHLIGHT + name + RESET " is not connected");
    }
    void user_lookup_error_log(const std::string& name)
    {
        log("ERR","User " HIGHLIGHT + name + RESET " not found");
    }
    void lookup_error_log(const std::string& host)
    {
        log("ERR","Hostname " HIGHLIGHT + host + RESET " not found");
    }
    void no_server_available_log()
    {
        log("ERR","You are not connected to any peer");
    }
    void command_not_found_log(const std::string& line)
    {
        log("ERR","Command \"" HIGHLIGHT + line + RESET "\" not found");
    }
}