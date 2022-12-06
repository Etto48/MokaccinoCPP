#include "commands.hpp"
#include "../../defines.hpp"
#include "../../parsing/parsing.hpp"
#include "../../logging/logging.hpp"
#include "../../network/udp/udp.hpp"
#include "../../network/udp/DataMap/DataMap.hpp"
#include "../../network/messages/messages.hpp"
#include "../../network/connection/connection.hpp"
#include "../../audio/audio.hpp"

namespace terminal::commands
{
    bool connect(const std::vector<std::string>& args)
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

    bool disconnect(const std::vector<std::string>& args)
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

    bool msg(const std::vector<std::string>& args)
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

    bool voice(const std::vector<std::string>& args)
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
};