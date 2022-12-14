#include "defines.hpp"
#include "ansi_escape.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <boost/program_options.hpp>
#include "multithreading/multithreading.hpp"
#include "logging/supervisor/supervisor.hpp"
#include "logging/logging.hpp"
#include "terminal/terminal.hpp"
#include "network/timecheck/timecheck.hpp"
#include "network/connection/connection.hpp"
#include "network/messages/messages.hpp"
#include "network/audio/audio.hpp"
#include "network/file/file.hpp"
#include "network/udp/udp.hpp"
#include "network/udp/crypto/crypto.hpp"
#include "network/authentication/authentication.hpp"
#include "parsing/parsing.hpp"
#include "ui/ui.hpp"
#include "toml.hpp"

bool DEBUG = 
#ifdef _DEBUG
    true;
#else
    false;
#endif

#ifdef _TEST
extern int test();
extern toml::table test_config;
#endif

bool config_loaded = false;

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
            "print help message and exit")
        ("version,v",
            "print version number and exit")
        ("port,p",boost::program_options::value<uint16_t>()->default_value(DEFAULT_PORT)->value_name("port"),
            "set a custom port for network communications")
        ("username,u",boost::program_options::value(&username)->value_name("username"),
            "select a username")
        ("config,c",boost::program_options::value<std::string>()->default_value(CONFIG_PATH)->value_name("config location"),
            "specify the path of the config file")
        ("log,l",boost::program_options::value<std::string>()->default_value("")->value_name("log file"),
            "enable logging to a file and specify the path")
        ("debug",boost::program_options::bool_switch()->value_name("debug log"),
        #ifdef _DEBUG
            "disable debug messages"
        #else
            "enable debug messages"
        #endif
        );
    boost::program_options::store(boost::program_options::parse_command_line(argc,argv,desc),args);
    boost::program_options::notify(args);
    if(args.count("help"))
    {
        std::cout << desc << std::endl;
    }
    else if(args.count("version"))
    {
        std::cout << "Mokaccino " << BUILD_TAG_VERSION << std::endl;
        std::cout << std::endl;
        std::cout << "Copyright (c) 2022 Ettore Ricci" << std::endl;
    }
    else
    {
        ui::init();
        logging::init(args["log"].as<std::string>());
        terminal::init();
        if(args["debug"].as<bool>())
        {
            DEBUG = not DEBUG;
        }
        //LOAD CONFIG
        toml::table config;
        try{
            #ifdef _TEST
            config = test_config;
            #else
            config = toml::parse_file(args["config"].as<std::string>());
            #endif
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
                std::cout << "Config file parsing error:" << std::endl;
                std::cout << "    " << e.what() << std::endl;
                std::cout << "    " << *e.source().path << " (" << e.source().begin.line << ", " << e.source().begin.column << ")" << std::endl;
                return -1;
            }
        }
        logging::set_time_format(config["terminal"]["time_format"].value_or<std::string>(DEFAULT_TIME_FORMAT)+RESET);
        
        username = config["network"]["username"].value_or(username);
        if(username.length() == 0)
        {
            logging::log("ERR","Missing required configuration for username");
            username = terminal::blocking_input("Username:");

            std::filesystem::create_directories(MOKACCINO_ROOT);
            std::fstream config_file{CONFIG_PATH,std::ios::out | std::ios::trunc};
            if(config.find("network") != config.end())
            {
                auto network_cfg = config["network"].as_table();
                if(network_cfg != nullptr)
                {
                    if(network_cfg->find("username") != network_cfg->end())
                        *network_cfg->find("username")->second.as_string() = username;
                    else
                        network_cfg->emplace("username",username);
                }
                else
                {
                    config.erase("network");
                    config.emplace("network",toml::table{ {"username", username} });
                }
            }
            else
                config.emplace("network",toml::table{ {"username", username} });
            config_file << config;
            logging::log("MSG","Config file written to \"" HIGHLIGHT + args["config"].as<std::string>() + RESET "\"");
        }

        std::vector<std::string> connection_whitelist = {};
        auto connection_whitelist_config = config["network"]["connection"]["whitelist"].as_array();
        if(connection_whitelist_config != nullptr)
        {
            for(auto&& p : *connection_whitelist_config)
            {
                if(p.is_string())
                    connection_whitelist.emplace_back(p.value_or(std::string{}));
            }
        }
        std::vector<std::string> audio_whitelist = {};
        auto audio_whitelist_config = config["network"]["audio"]["whitelist"].as_array();
        if(audio_whitelist_config != nullptr)
        {
            for(auto&& p : *audio_whitelist_config)
            {
                if(p.is_string())
                    audio_whitelist.emplace_back(p.value_or(std::string{}));
            }
        }

        //INITIALIZATIONS
        logging::supervisor::init(60);
        network::authentication::init();
        network::udp::init(config["network"]["port"].value_or(args["port"].as<uint16_t>()));
        auto encryption = config["network"]["connection"]["encrypt_by_default"].value_or(true);
        if(not encryption)
            logging::log("MSG","Encryption " HIGHLIGHT "disabled" RESET);
        else    
            logging::log("MSG","Encryption " HIGHLIGHT "enabled" RESET);
        network::connection::init(
            username,
            connection_whitelist,
            config["network"]["connection"]["default_action"].value_or(std::string("prompt")),
            encryption);
        network::messages::init();
        network::timecheck::init();
        network::audio::init(audio_whitelist,config["network"]["audio"]["default_action"].value_or(std::string("prompt")),config["network"]["audio"]["threshold"].value_or<int16_t>(40));
        network::file::init();

        //autoconnection
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

        //startup_commands execution
        auto startup_commands_config = config["terminal"]["startup_commands"].as_array();
        if(startup_commands_config != nullptr) 
        {// if startup_commands was defined in the config
            for(auto&& c : *startup_commands_config)
            {
                if(c.is_string())
                {
                    auto command = c.value_or(std::string{});
                    terminal::process_command(command);
                }
            }
        }
        //tests
        #ifdef _TEST
        int ret = test();
        multithreading::request_termination();
        #else
        int ret = 0;
        #endif

        //WAIT FOR EXIT
        multithreading::wait_termination();
        multithreading::stop_all();
        multithreading::join();
        ui::fini();
        return ret;
    }
    return 0;
}