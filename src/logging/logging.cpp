#include "logging.hpp"
#ifdef _DEBUG 
    #define VERBOSITY 2
#else
    #define VERBOSITY 0
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
            std::cout << terminal_prefix << message << std::endl;
        }
    }
    
    void message_log(std::string src, std::string message)
    {
        log("DBG",_format("Message from " HIGHLIGHT "{}" RESET " received: \"" HIGHLIGHT "{}" RESET "\"",src,message));
    }
    void new_thread_log(std::string thread_name)
    {
        log("DBG",_format("Thread " HIGHLIGHT "{}" RESET " started",thread_name));
    }
    void supervisor_log(size_t connections,size_t services)
    {
        log("DBG",_format(TAG "[SUPERVISOR]" RESET " connections:" HIGHLIGHT "{}" RESET " services:" HIGHLIGHT "{}" RESET,connections,services));
    }
    void new_user_log(std::string name, const boost::asio::ip::udp::endpoint& endpoint)
    {
        log("DBG",_format("User " HIGHLIGHT "{}" RESET ", (" HIGHLIGHT "{}" RESET ":" HIGHLIGHT "{}" RESET ") added",name,endpoint.address().to_string(),endpoint.port()));
    }
    void removed_user_log(std::string name)
    {
        log("DBG",_format("User " HIGHLIGHT "{}" RESET " removed",name));
    }
    void connection_test_log(const network::MessageQueueItem& item)
    {
        log("DBG",_format(TAG "[CONNECTION]" RESET " Handled test message \"" HIGHLIGHT "{}" RESET "\" from " HIGHLIGHT "{}" RESET "@" HIGHLIGHT "{}" RESET ":" HIGHLIGHT "{}" RESET,item.msg,item.src,item.src_endpoint.address().to_string(),item.src_endpoint.port()));
    }
    void terminal_processing_log(const std::string& line)
    {
        log("DBG",_format(TAG "[TERMINAL]" RESET "Processing command \"" HIGHLIGHT "{}" RESET "\"",line));
    }
}