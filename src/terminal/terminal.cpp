#include "terminal.hpp"
#include "../defines.hpp"
#include "../ansi_escape.hpp"
#include <iostream>
#include <boost/asio/ip/udp.hpp>
#include "../parsing/parsing.hpp"
#include "../network/connection/connection.hpp"
#include "../network/messages/messages.hpp"
#include "../network/udp/udp.hpp"
#include "../multithreading/multithreading.hpp"
#include "../logging/logging.hpp"
#include "commands/commands.hpp"
#include "prompt.hpp"
namespace terminal
{
    std::mutex command_function_mutex;
    std::map<std::string,CommandFunction> command_function_map;
    void add_command(const CommandFunction& command_info)
    {
        if(command_info.name != "exit" and command_info.name != "help" and command_info.function != nullptr)
        {
            std::unique_lock lock(command_function_mutex);
            command_function_map[command_info.name] = command_info;
            logging::log("DBG","Command \"" HIGHLIGHT + command_info.name + RESET "\" added");
        }
    }
    bool exit_called = false;
    bool process_command(const std::string& line)
    {
        logging::terminal_processing_log(line);
        auto args = parsing::cmd_args(line);
        if(args.size() == 1 and args[0] == "exit")
        {
            multithreading::request_termination();
            exit_called = true;
            return true;
        }
        else if((args.size() >= 1 or args.size() <= 2) and args[0] == "help")
        {
            std::unique_lock command_lock(command_function_mutex);
            if(args.size() == 1)
            {//list all commands
                for(auto& [command_name,info]: command_function_map)
                {
                    logging::log("MSG","- " + info.name);
                    logging::log("MSG","\t" + info.help_text);
                    logging::log("MSG","");
                }
                logging::log("MSG","- help");
                logging::log("MSG","\tShow a list of commands or help about a specific command");
                logging::log("MSG","");
                logging::log("MSG","- exit");
                logging::log("MSG","\tClose the program");
                logging::log("MSG","");
                logging::log("MSG","Use help <command> to show more info about a specific command");
                return true;
            }
            else
            {//provide help about one command
                auto command_info_iter = command_function_map.find(args[1]);
                if(command_info_iter != command_function_map.end())
                {
                    auto& info = command_info_iter->second;
                    logging::log("MSG","- " + info.name);
                    logging::log("MSG","\tUsage: "+info.name+" "+info.usage);
                    logging::log("MSG","\t" + info.help_text);
                    return true;
                }
                else if(args[1] == "help")
                {
                    logging::log("MSG","- help");
                    logging::log("MSG","\tUsage: help [command]");
                    logging::log("MSG","\tShow a list of commands or help about a specific command");
                    return true;
                }
                else if(args[1] == "exit")
                {
                    logging::log("MSG","- exit");
                    logging::log("MSG","\tUsage: exit");
                    logging::log("MSG","\tClose the program");
                    return true;
                }
                else
                {
                    logging::log("ERR",args[1] + " is not a valid command");
                    return false;
                }
            }
        }
        else
        {
            std::unique_lock command_lock(command_function_mutex);
            auto command_info_iter = command_function_map.find(args[0]);
            if(command_info_iter != command_function_map.end())
            {
                auto& cmd_info = command_info_iter->second;
                if((cmd_info.min_argn == 0 or cmd_info.min_argn <= args.size()) and (cmd_info.max_argn == 0 or cmd_info.max_argn >= args.size()))
                {//successfully called the command
                    return (*cmd_info.function)(args);
                }
                else
                {//syntax error
                    logging::log("ERR","Syntax error");
                    logging::log("ERR","\tUsage: "+cmd_info.name+" "+cmd_info.usage);
                    return false;
                }
            }
            else
            {
                logging::command_not_found_log(line);
                return false;
            }
        }
    }
    
    void terminal()
    {
        #ifndef _TEST
        if(not IsDebuggerPresent())
        {
            while(not exit_called)
            {
                {
                    std::unique_lock out_lock(logging::output_mutex);
                    prompt();
                }
                std::string line;
                std::getline(std::cin,line);
                if(line.size() > 0)
                    last_ret = process_command(line);
            }
        }
        #endif
    }
    void init()
    {
        add_command(CommandFunction{
            "connect",
            "(hostname <hostname>)|(name <username>)",
            "Connect to a peer with a knwon hostname or with a normal peer depending if hostname or name is specified",
            commands::connect,
            3,3});
        add_command(CommandFunction{
            "disconnect",
            "<username>",
            "Disconnect from the specified user",
            commands::disconnect,
            2,2});
        add_command(CommandFunction{
            "msg",
            "<username> <message>",
            "Send a text message to the specified user",
            commands::msg,
            3,3});
        add_command(CommandFunction{
            "voice",
            "(start <username>)|(stop)",
            "Start a voice call with a user or stop one previously started",
            commands::voice,
            2,3});
        add_command(CommandFunction{
            "sleep",
            "<seconds>",
            "Make the calling process sleep for a number of seconds (useful with config files)",
            commands::sleep,
            2,2});
        add_command(CommandFunction{
            "user",
            "(list)|(info <username>)",
            "Get a list of connected users or the address about a specific user",
            commands::user,
            2,3});
        multithreading::add_service("terminal",terminal);
    }
}