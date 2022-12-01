#include "defines.hpp"
#include <iostream>
#include <boost/program_options.hpp>
#include "network/udp/udp.hpp"
#include "multithreading/multithreading.hpp"
#include "logging/supervisor/supervisor.hpp"
#include "terminal/terminal.hpp"
#include "network/connection/connection.hpp"
#include "network/messages/messages.hpp"

#include "toml.hpp"

int main(int argc, char* argv[])
{
    //PROGRAM OPTIONS
    boost::program_options::options_description desc {
        "Mokaccino is a simple P2P application that supports VoIP and text messages\n"
        "Allowed options (overrides the config file)"
    };
    boost::program_options::variables_map args;

    std::string username;

    desc.add_options()
        ("help,h",
            "print help message")
        ("port,p",boost::program_options::value<uint16_t>()->default_value(DEFAULT_PORT)->value_name("port"),
            "set a custom port for network communications")
        ("username",boost::program_options::value(&username),
            "select a username");
    boost::program_options::store(boost::program_options::parse_command_line(argc,argv,desc),args);
    boost::program_options::notify(args);
    if(args.count("help"))
    {
        std::cout << desc << std::endl;
    }
    else
    {
        //LOAD CONFIG
        toml::table config;
        try{
            config = toml::parse_file(CONFIG_PATH);
        }catch(const toml::parse_error& e)
        {
            if(std::string(e.what()) == "File could not be opened for reading")
            {
                logging::config_not_found_log(CONFIG_PATH);
            }
            else
            {
                logging::config_error_log(e);
            }
            return -1;
        }

        //INITIALIZATIONS
        logging::supervisor::init(60);
        network::udp::init(config["network"]["port"].value_or(args["port"].as<uint16_t>()));
        network::connection::init(config["network"]["username"].value_or(username));
        network::messages::init();
        terminal::init();
        
        auto autoconnect_peers_config = *config["network"]["autoconnect"].as_array();
        for(auto&& p : autoconnect_peers_config)
        {
            if(p.is_string())
            {
                auto peer_address = p.value_or(std::string{});
                auto endpoint_host = parsing::endpoint_from_hostname(peer_address);
                network::connection::connect(endpoint_host.first,endpoint_host.second);
            }
        }

        
        //tests
        #ifdef _DEBUG
        network::udp::send("TEST","loopback");
        terminal::process_command("connect hostname localhost");
        network::messages::send("loopback","test message with spaces");
        #endif

        //WAIT FOR EXIT
        multithreading::wait_termination();
        multithreading::stop_all();
        multithreading::join();
    }
    return 0;
}