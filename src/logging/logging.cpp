#include "logging.hpp"
#ifdef DEBUG 
    #define VERBOSITY 2
#else
    #define VERBOSITY 0
#endif
namespace logging
{
    void log(std::string log_type,std::string message)
    {
        std::string terminal_prefix;
        std::string file_prefix;
        unsigned int verbosity_required = 0;
        if(log_type=="ERR")
        {
            file_prefix = "[ERROR] ";
            terminal_prefix = "\033[032m[ERROR]\033[0m ";
            verbosity_required = 0;
        }else if(log_type == "DBG")
        {
            file_prefix = "[DEBUG] ";
            terminal_prefix = "\033[033m[DEBUG]\033[0m ";
            verbosity_required = 2;
        }else if(log_type == "MSG")
        {
        
            file_prefix = "[MESSAGE] ";
            terminal_prefix = "\033[034m[MESSAGE]\033[0m ";
            verbosity_required = 1;
        }
        if(VERBOSITY >= verbosity_required)
        {
            std::cout << terminal_prefix << message << std::endl;
        }
    }
    #define HIGHLIGHT "\033[35m"
    #define RESET "\033[0m"
    void message_log(std::string src, std::string message)
    {
        log("DBG",std::format("Message from " HIGHLIGHT "{}" RESET " received: \"" HIGHLIGHT "{}" RESET "\"",src,message));
    }
    void new_thread_log(std::string thread_name)
    {
        log("DBG",std::format("Thread " HIGHLIGHT "{}" RESET " started",thread_name));
    }
}