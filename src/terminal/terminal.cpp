#include "terminal.hpp"
namespace terminal
{
    bool exit_called = false;
    bool process_command(const std::string& line)
    {
        logging::terminal_processing_log(line);
        auto args = parsing::cmd_args(line);
        if(args.size() == 1 && args[0] == "exit")
        {
            multithreading::request_termination();
            exit_called = true;
            return true;
        }
        else if(args.size() == 3 && args[0] == "connect")
        {
            if(args[1] == "hostname")
            {
                std::string host, port = DEFAULT_PORT_STR;
                uint16_t port_var = DEFAULT_PORT;
                if(args[2].find(':') != std::string::npos)
                {
                    host = args[2].substr(0,args[2].find(':'));
                    port = args[2].substr(args[2].find(':')+1,args[2].length()-host.length()-1);
                }
                else
                {
                    host = args[2];
                }
                try{
                    port_var = std::stoi(port);
                }catch(std::exception&)
                {
                    port_var = DEFAULT_PORT;
                }

                try
                {
                    auto endpoint = network::udp::dns_lookup(host,port_var);
                    logging::log("DBG","Target found at "+endpoint.address().to_string()+":"+std::to_string(endpoint.port()));
                    network::connection::connect(endpoint, host);
                }catch(network::udp::LookupError)
                {
                    logging::lookup_error_log(host);
                    return false;
                }
            }
            else if(args[1] == "name")
            {
                try
                {
                    network::udp::server_request(args[2]);
                }catch(network::DataMap::NotFound)
                {
                    logging::no_server_available_log();
                    return false;
                }
            }
            return true;
        }
        else if(args.size() == 2 && args[0] == "disconnect")
        {
            if(network::udp::connection_map.check_user(args[1]))
            {
                network::udp::send(parsing::compose_message({"DISCONNECT","connection closed"}),args[1]);
                network::udp::connection_map.remove_user(args[1]);
                logging::sent_disconnect_log(args[1]);
                return true;
            }
            else
            {
                logging::user_not_found_log(args[1]);
                return false;
            }
        }
        else if(args.size() == 3 && args[0] == "msg")
        {
            if(network::udp::connection_map.check_user(args[1]))
            {
                network::messages::send(args[1],args[2]);
                return true;
            }
            else
            {
                logging::user_not_found_log(args[1]);
                return false;
            }
        }
        else
        {
            logging::command_not_found_log(line);
            return false;
        }
    }
    
    void terminal()
    {
        while(!exit_called)
        {
            prompt();
            std::string line;
            std::getline(std::cin,line);
            if(line.size() > 0)
                last_ret = process_command(line);
        }
    }
    void init()
    {
        multithreading::add_service("terminal",terminal);
    }
}