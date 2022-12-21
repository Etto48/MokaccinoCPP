#include "logging.hpp"
#include "../defines.hpp"
#include "../ansi_escape.hpp"
#include <iostream>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdint.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include "../terminal/terminal.hpp"
#include "../multithreading/multithreading.hpp"
#include "../parsing/parsing.hpp"
#include "../ui/ui.hpp"

namespace logging
{
    std::mutex output_mutex;
    std::string path_to_log;
    std::string clock_format = DEFAULT_TIME_FORMAT;
    void init(const std::string& log_file)
    {
        path_to_log = log_file;
        if(path_to_log.length() > 0)
        {
            std::fstream file(path_to_log,std::ios::out | std::ios::app);
            if(not file.is_open())
            {
                logging::log("ERR","Error opening log file \"" HIGHLIGHT +log_file+ RESET "\"");
                path_to_log = "";
            }else
            {
                file << "--- Mokaccino log started ---" << std::endl;
            }
        }
        logging::log("DBG","Logging initialized");
    }
    void set_time_format(const std::string& time_format)
    {
        logging::clock_format = time_format;
    }
    void log(std::string log_type,std::string message)
    {
        static std::string last_prefix;
        static std::string last_time;
        unsigned int verbosity = DEBUG? 2 : 1;
        std::string terminal_prefix;
        std::string file_prefix;
        std::string time;
        #ifndef NO_CLOCK
        auto* facet = new boost::posix_time::time_facet{clock_format.c_str()};
        std::stringstream ss;
        std::locale locale{ss.getloc(),facet};
        ss.imbue(locale);
        ss << boost::posix_time::second_clock().local_time();
        time = ss.str();
        #endif
        unsigned int verbosity_required = 0;
        if(log_type=="ERR")
        {
            file_prefix = "[ERROR] ";
            terminal_prefix = ERR_TAG "[ERROR]" RESET " ";
            verbosity_required = 0;   
        }else if(log_type=="PRM")
        {
            file_prefix = "[PROMPT] ";
            terminal_prefix = PROMPT_TAG "[PROMPT]" RESET " ";
            verbosity_required = 0;
        }else if(log_type == "MSG")
        {
            file_prefix = "[MESSAGE] ";
            terminal_prefix = MSG_TAG "[MESSAGE]" RESET " ";
            verbosity_required = 1;
        }else if(log_type == "DBG")
        {
            auto thread_name = multithreading::get_current_thread_name();
            file_prefix = "[DEBUG] ["+thread_name+"] ";
            terminal_prefix = DBG_TAG "[DEBUG]" RESET " " TAG "[" + thread_name + "]" RESET " ";
            verbosity_required = 2;
        }
        if(verbosity >= verbosity_required)
        {
            std::unique_lock lock(output_mutex);
            if(log_type == "PRM")
            {
                #ifdef NO_ANSI_ESCAPE
                std::cout << terminal_prefix << parsing::strip_ansi(message) << ": ";
                std::cout.flush();
                #else
                #ifdef NO_TERMINAL_UI
                std::cout << "\r" CLEAR_LINE << terminal_prefix << message << ": ";
                std::cout.flush();
                #else
                ui::prompt(RESET+message);
                #endif
                #endif
            }
            else
            {
                if(time == last_time) 
                {// skip timestamp
                    time = std::string(parsing::strip_ansi(last_time).length(),' ');
                }
                else
                    last_time = time;

                if(terminal_prefix == last_prefix)
                {//skip terminal prefix
                    terminal_prefix = std::string(parsing::strip_ansi(last_prefix).length(),' ');
                    file_prefix = terminal_prefix;
                }
                else
                    last_prefix = terminal_prefix;

                

                #ifdef NO_ANSI_ESCAPE
                std::cout << time << terminal_prefix << parsing::strip_ansi(message) << std::endl;
                #else
                #ifdef NO_TERMINAL_UI
                std::cout << "\r" CLEAR_LINE << time << terminal_prefix << message << std::endl;
                #else
                ui::print(time + terminal_prefix, message);
                #endif
                #endif
                if(path_to_log.length() > 0)
                {
                    std::fstream file(path_to_log,std::ios::out | std::ios::app);
                    file << parsing::strip_ansi(time) << file_prefix << parsing::strip_ansi(message) << std::endl;
                }
                if(not IsDebuggerPresent())
                    terminal::prompt();
            }
        }
    }
    
    void message_log(std::string src, std::string message)
    {
        log("DBG", "Message from " HIGHLIGHT + src + RESET " received: \"" HIGHLIGHT + message + RESET "\"");
    }
    void new_thread_log(std::string thread_name)
    {
        log("DBG", "Thread " HIGHLIGHT + thread_name + RESET " started");
    }
    void new_user_log(std::string name, const boost::asio::ip::udp::endpoint& endpoint)
    {
        log("DBG", "User " HIGHLIGHT + name + RESET ", (" HIGHLIGHT + endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET ") added");
    }
    void removed_user_log(std::string name)
    {
        log("DBG", "User " HIGHLIGHT + name + RESET " removed");
    }
    void connection_test_log(const network::MessageQueueItem& item)
    {
        log("DBG", "Handled test message \"" HIGHLIGHT + item.msg + RESET "\" from " HIGHLIGHT + item.src + RESET "@" HIGHLIGHT + item.src_endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(item.src_endpoint.port()) + RESET);
    }
    void terminal_processing_log(const std::string& line)
    {
        log("DBG", "Processing command \"" HIGHLIGHT + line + RESET "\"");
    }
    void config_success_log(const std::string& path)
    {
        log("DBG", "Config file loaded from \"" HIGHLIGHT + path + RESET "\"");
    }

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
        log("ERR","Command \"" HIGHLIGHT + line + RESET "\" not found, use \"help\" for a list of commands");
    }
    void config_not_found_log(const std::string& path)
    {
        log("ERR","Config file \"" HIGHLIGHT + path + RESET "\" not found");
    }
    void config_error_log(const toml::parse_error& e)
    {
        log("ERR","Config file parsing error:");
        log("ERR",std::string("    ") + e.what());
        log("ERR",std::string("    ") + (*e.source().path) + " (" + std::to_string(e.source().begin.line) + ", " + std::to_string(e.source().begin.column) + ")");
    }
    void audio_call_error_log(const std::string& why)
    {
        log("ERR","Error during audio initialization (" HIGHLIGHT + why + RESET ")");
    }
}