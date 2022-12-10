#include "terminal.hpp"
#include "../defines.hpp"
#include "../ansi_escape.hpp"
#include <iostream>
#include <queue>
#include <semaphore>
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
    std::mutex input_queue_mutex;
    std::queue<std::pair<std::string,std::function<void(const std::string&)>>> input_queue;
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
        if(line.length() == 0) return true;
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
                    logging::log("MSG","- " HIGHLIGHT + info.name + RESET);
                    logging::log("MSG","    " + info.help_text);
                    logging::log("MSG","");
                }
                logging::log("MSG","- " HIGHLIGHT "help" RESET);
                logging::log("MSG","    Show a list of commands or help about a specific command");
                logging::log("MSG","");
                logging::log("MSG","- " HIGHLIGHT "exit" RESET);
                logging::log("MSG","    Close the program");
                logging::log("MSG","");
                logging::log("MSG","Use \"help <command>\" to show more info about a specific command");
                return true;
            }
            else
            {//provide help about one command
                auto command_info_iter = command_function_map.find(args[1]);
                if(command_info_iter != command_function_map.end())
                {
                    auto& info = command_info_iter->second;
                    logging::log("MSG","- " HIGHLIGHT + info.name + RESET);
                    logging::log("MSG","    Usage: "+info.name+" "+info.usage);
                    logging::log("MSG","    " + info.help_text);
                    return true;
                }
                else if(args[1] == "help")
                {
                    logging::log("MSG","- " HIGHLIGHT "help" RESET);
                    logging::log("MSG","    Usage: help [command]");
                    logging::log("MSG","    Show a list of commands or help about a specific command");
                    return true;
                }
                else if(args[1] == "exit")
                {
                    logging::log("MSG","- " HIGHLIGHT "exit" RESET);
                    logging::log("MSG","    Usage: exit");
                    logging::log("MSG","    Close the program");
                    return true;
                }
                else
                {
                    logging::log("ERR","\"" HIGHLIGHT + args[1] + RESET "\" is not a valid command");
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
                    return (*cmd_info.function)(line,args);
                }
                else
                {//syntax error
                    logging::log("ERR","Syntax error");
                    logging::log("ERR","    Usage: "+cmd_info.name+" "+cmd_info.usage);
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
                {
                    bool run_command = false;
                    {
                        std::unique_lock queue_lock(input_queue_mutex);
                        if(not input_queue.empty())
                        {
                            auto callback = input_queue.front();
                            input_queue.pop();
                            callback.second(line);
                            if(not input_queue.empty())
                            {
                                auto prompt = input_queue.front().first;
                                logging::log("PRM",prompt);
                            }
                        }
                        else
                        {
                            run_command = true;
                        }
                    }
                    if(run_command)
                        last_ret = process_command(line);
                }
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
            "Send a text message to the specified user, you can write the message without escaping whitespaces",
            commands::msg,
            3,0});
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
        #ifndef NO_TERMINAL_UI
        add_command(CommandFunction{
            "scroll",
            "(up|down) [lines]",
            "Scroll the terminal up or down one ore more lines",
            commands::scroll,
            2,3});
        #endif
        multithreading::add_service("terminal",terminal);
    }
    void input(const std::string& prompt, std::function<void(const std::string&)> callback)
    {
        if(callback != nullptr)
        {
            std::unique_lock lock(input_queue_mutex);
            if(input_queue.empty())
            { // we print the prompt
                logging::log("PRM",prompt);   
            }
            input_queue.push({prompt,callback});
        }
    }

    std::string blocking_input(const std::string& prompt)
    {
        std::counting_semaphore sync_sem{0};
        std::string ret;
        input(prompt,
            [&sync_sem,&ret](const std::string& input){
                ret = input;
                sync_sem.release();
            });
        sync_sem.acquire();
        return ret;
    }
}