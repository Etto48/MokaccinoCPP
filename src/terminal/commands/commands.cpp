#include "commands.hpp"
#include "../../defines.hpp"
#include "../../ansi_escape.hpp"
#include <boost/thread.hpp>
#include "../../parsing/parsing.hpp"
#include "../../logging/logging.hpp"
#include "../../network/udp/udp.hpp"
#include "../../network/udp/DataMap/DataMap.hpp"
#include "../../network/messages/messages.hpp"
#include "../../network/connection/connection.hpp"
#include "../../audio/audio.hpp"

namespace terminal::commands
{
    bool connect(const std::string& line, const std::vector<std::string>& args)
    {
        if(args[1] == "hostname")
        {
            try
            {
                auto endpoint_hostname = parsing::endpoint_from_hostname(args[2]);
                auto endpoint = endpoint_hostname.first;
                auto host = endpoint_hostname.second;
                logging::log("DBG","Target found at "+endpoint.address().to_string()+":"+std::to_string(endpoint.port()));
                network::connection::connect(endpoint, host);
            }catch(network::udp::LookupError)
            {
                logging::lookup_error_log(args[2]);
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
        else
        {
            logging::log("ERR","The second argument must be hostname or name, use \"help connect\" for more info");
            return false;
        }
        return true;
    }

    bool disconnect(const std::string& line, const std::vector<std::string>& args)
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

    bool msg(const std::string& line, const std::vector<std::string>& args)
    {
        if(network::udp::connection_map.check_user(args[1]))
        {
            std::string message_text = line.substr(line.find(args[0]));
            message_text = message_text.substr(message_text.find(args[1]));
            message_text = message_text.substr(message_text.find(args[2]));
            network::messages::send(args[1],message_text);
            return true;
        }
        else
        {
            logging::user_not_found_log(args[1]);
            return false;
        }
    }

    bool voice(const std::string& line, const std::vector<std::string>& args)
    {
        if(args[1] == "start")
        {
            if(args.size() == 3)
            {
                //TODO: check if all went right, user found and not already in a call
                audio::start_call(args[2]);
                return true;
            }else
            {
                logging::log("ERR","No username provided, use \"help voice\" for more info");
                return false;
            }
        }
        else if(args[1] == "stop")
        {
            if(args.size() == 2)
            {
                //TODO: check if all went right, must be already in a call to do this
                audio::stop_call();
                return true;
            }else
            {
                logging::log("ERR","Too many arguments, use \"help voice\" for more info");
                return false;
            }
        }else
        {
            logging::log("ERR","The second argument must be start or stop, use \"help voice\" for more info");
            return false;
        }
    }

    bool sleep(const std::string& line, const std::vector<std::string>& args)
    {
        try
        {
            auto seconds = std::stoul(args[1]);
            logging::log("DBG","Sleeping for " HIGHLIGHT + args[1] + RESET "s");
            boost::this_thread::sleep_for(boost::chrono::seconds(seconds));
            return true;
        }catch(std::invalid_argument&)
        {
            logging::log("ERR","The argument must be a positive number");
            return false;
        }
    }

    bool user(const std::string& line, const std::vector<std::string>& args)
    {
        if(args[1] == "list")
        {
            if(args.size() == 2)
            {
                auto users = network::udp::connection_map.get_connected_users();
                for(auto& [name, endpoint]: users)
                {
                    logging::log("MSG",HIGHLIGHT + name + RESET);
                }
                return true;
            }else
            {
                logging::log("ERR","Too many arguments, use \"help user\" for more info");
                return false;
            }
        }else if(args[1] == "info")
        {
            if(args.size() == 3)
            {
                try{
                    auto user_info = network::udp::connection_map[args[2]];
                    logging::log("MSG",HIGHLIGHT + user_info.name + RESET);
                    logging::log("MSG","\tAddress: " HIGHLIGHT + user_info.endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(user_info.endpoint.port()) + RESET);
                    auto ms = user_info.avg_latency.total_milliseconds() / 2;
                    if(ms != 0)
                        logging::log("MSG","\tPing: " HIGHLIGHT + std::to_string(ms) + RESET "ms");
                    else
                    {
                        auto us = user_info.avg_latency.total_microseconds() / 2;
                        logging::log("MSG","\tPing: " HIGHLIGHT + std::to_string(us) + RESET "us");
                    }
                    return true;
                }catch(network::DataMap::NotFound&)
                {
                    logging::log("ERR","User not connected, use \"help user\" and \"help connect\" for more info");
                    return false;
                }
            }
            else
            {
                logging::log("ERR","No username was provided, use \"help user\" for more info");
                return false;
            }

        }else
        {
            logging::log("ERR","The second argument must be list or info, use \"help user\" for more info");
            return false;
        }
    }
};