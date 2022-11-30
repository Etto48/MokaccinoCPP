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
        else if((args.size() == 3) && args[0] == "connect")
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
                    logging::log("DBG",std::format("Target found at {}:{}",endpoint.address().to_string(),endpoint.port()));
                    network::connection::connect(endpoint, host);
                }catch(network::udp::LookupError)
                {
                    std::cout << "Error resolving hostname" << std::endl;
                    logging::log("DBG",std::format("{} not found",host));
                    return false;
                }
            }
            else if(args[1] == "name")
            {
                try
                {
                    network::udp::server_request(args[2]);
                    std::cout << "Waiting for the other peer" << std::endl;
                }catch(network::DataMap::NotFound)
                {
                    std::cout << "You are not connected to a suitable peer" << std::endl;
                    logging::log("DBG","No server available");
                }
            }
            return true;
        }
        else
        {
            std::cout<<"Command not found"<<std::endl;
            return false;
        }
    }
    void terminal()
    {
        bool last_ret = true;
        while(!exit_called)
        {
            if(last_ret)
                std::cout << "\r\033[K\033[32mO\033[0m> ";
            else
                std::cout << "\r\033[K\033[31mX\033[0m> ";
            std::cout.flush();
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