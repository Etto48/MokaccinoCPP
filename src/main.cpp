#include "defines.hpp"
#include <iostream>
#include <boost/program_options.hpp>
#include "network/udp/udp.hpp"
#include "multithreading/multithreading.hpp"
#include "logging/supervisor/supervisor.hpp"
#include "terminal/terminal.hpp"
#include "network/connection/connection.hpp"
#include "network/messages/messages.hpp"
#include "audio/audio.hpp"

#include "toml.hpp"

bool DEBUG = false;

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
        ("username,u",boost::program_options::value(&username)->value_name("username"),
            "select a username")
        ("config,c",boost::program_options::value<std::string>()->default_value(CONFIG_PATH)->value_name("config location"),
            "specify the path of the config file")
        ("debug",boost::program_options::bool_switch(&DEBUG),
            "enable debug messages and tests");
    boost::program_options::store(boost::program_options::parse_command_line(argc,argv,desc),args);
    boost::program_options::notify(args);
    if(args.count("help"))
    {
        std::cout << desc << std::endl;
    }
    else
    {
        #ifdef _DEBUG
            DEBUG = true;
        #endif
        //LOAD CONFIG
        toml::table config;
        bool config_loaded = false;
        try{
            config = toml::parse_file(args["config"].as<std::string>());
            logging::config_success_log(args["config"].as<std::string>());
            config_loaded = true;
        }catch(const toml::parse_error& e)
        {
            if(std::string(e.what()) == "File could not be opened for reading")
            {
                logging::config_not_found_log(args["config"].as<std::string>());
            }
            else
            {
                logging::config_error_log(e);
                return -1;
            }
        }
        
        if(!config_loaded && username.length() == 0)
        {
            std::cout << "\r\033[KUsername: ";
            std::cout.flush();
            std::cin >> username;
            logging::log("MSG","Consider providing a config file in \"" + args["config"].as<std::string>() + "\"");
        }


        //INITIALIZATIONS
        logging::supervisor::init(60);
        network::udp::init(config["network"]["port"].value_or(args["port"].as<uint16_t>()));
        network::connection::init(config["network"]["username"].value_or(username));
        network::messages::init();
        terminal::init();
        audio::init();
        
        auto autoconnect_peers_config = config["network"]["autoconnect"].as_array();
        if(autoconnect_peers_config != nullptr) 
        {// if autoconnect was defined in the config
            for(auto&& p : *autoconnect_peers_config)
            {
                if(p.is_string())
                {
                    auto peer_address = p.value_or(std::string{});
                    auto endpoint_host = parsing::endpoint_from_hostname(peer_address);
                    network::connection::connect(endpoint_host.first,endpoint_host.second);
                }
            }
        }

        
        //tests
        if(DEBUG)
        {
            network::udp::send("TEST","loopback");
            terminal::process_command("connect hostname localhost");
            network::messages::send("loopback","test message with spaces");
        }

        //WAIT FOR EXIT
        multithreading::wait_termination();
        multithreading::stop_all();
        multithreading::join();
    }
    return 0;
}