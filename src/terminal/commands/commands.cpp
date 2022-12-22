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
#include "../../network/audio/audio.hpp"
#include "../../network/authentication/authentication.hpp"
#include "../../network/file/file.hpp"
#include "../../ui/ui.hpp"

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
                logging::log("DBG","Target found at " HIGHLIGHT + endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET);
                return network::connection::connect(endpoint, host);
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
                return true;
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
                return network::audio::start_call(args[2]);
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
                return network::audio::stop_call();
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
                    logging::log("MSG","    Address: " HIGHLIGHT + user_info.endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(user_info.endpoint.port()) + RESET);
                    auto ms = user_info.avg_latency.total_milliseconds() / 2;
                    if(ms != 0)
                        logging::log("MSG","    Ping: " HIGHLIGHT + std::to_string(ms) + RESET "ms");
                    else
                    {
                        auto us = user_info.avg_latency.total_microseconds() / 2;
                        logging::log("MSG","    Ping: " HIGHLIGHT + std::to_string(us) + RESET "us");
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
    bool scroll(const std::string& line, const std::vector<std::string>& args)
    {
        if(args[1] == "up")
        {
            if(args.size() == 3)
            { // n lines
                try
                {
                    int l_count = std::stoi(args[2]);
                    if(l_count < 0)
                        throw std::exception{}; 
                    ui::scroll_text(l_count);
                }catch(std::exception&)
                {
                    logging::log("ERR","The argument must be a positive number");
                    return false;
                }
            }
            else
            { // one line
                ui::scroll_text(1);
            }
        }
        else if(args[1] == "down")
        { 
            if(args.size() == 3)
            { // n lines
                try
                {
                    int l_count = std::stoi(args[2]);
                    if(l_count < 0)
                        throw std::exception{};
                    ui::scroll_text(-l_count);
                }catch(std::exception&)
                {
                    logging::log("ERR","The argument must be a positive number");
                    return false;
                }
            }
            else
            { // one line
                ui::scroll_text(-1);
            }
        }
        else
        {
            logging::log("ERR","The second argument must be up or down, use \"help user\" for more info");
            return false;
        }
        return true;
    }
    bool file(const std::string& line, const std::vector<std::string>& args)
    {
        if(args[1] == "send")
        {
            if(args.size() != 4)
            {
                logging::log("ERR","You must provide the user and the file to send, use \"help file\" for more info");
                return false;
            }
            else
            {
                try
                {
                    if(not network::file::init_file_upload(args[2],args[3]))
                    {
                        logging::log("ERR","You are already sending or receiving a file with the same hash");
                        return false;
                    }   
                    else
                    {
                        return true;
                    }
                }
                catch(network::file::FileNotFound&)
                {
                    logging::log("ERR","Could not open file at \"" HIGHLIGHT + args[3] + RESET "\"");
                    return false;
                }
                catch(network::file::UserNotFound&)
                {
                    logging::user_not_found_log(args[2]);
                    return false;
                }
            }
        }else if(args[1] == "list")
        {
            if(args.size() != 2)
            {
                logging::log("ERR","No more arguments needed, use \"help file\" for more info");
                return false;
            }
            else
            {
                std::unique_lock lock(network::file::file_transfers_mutex);
                unsigned int i = 0;
                for(auto& [hash, info]: network::file::file_transfers)
                {
                    
                    logging::log("MSG","- " HIGHLIGHT + info.file_name + RESET);
                    logging::log("MSG","    " + std::string(info.direction == network::file::FileTransferDirection::download? "DOWNLOAD":"UPLOAD"));
                    logging::log("MSG","    ACKed " + std::to_string(info.last_acked_number) + "/" + std::to_string(info.data.size()) + " " + std::to_string(float(info.last_acked_number)/info.data.size()*100)+"%");
                    logging::log("MSG","    "+ std::string(info.direction == network::file::FileTransferDirection::download? "Received":"Sent") + " " + std::to_string(info.next_sequence_number) + "/" + std::to_string(info.data.size()) + " " + std::to_string(float(info.next_sequence_number)/info.data.size()*100)+"%");
                    logging::log("MSG","    Throughput " + std::to_string(info.average_throughput/1024*8) + "Kbps");
                    if(i!=network::file::file_transfers.size()-1)
                        logging::log("MSG","");
                    i++;
                }
                return true;
            }
        }
        else{
            logging::log("ERR","The second argument must be send or list, use \"help file\" for more info");
            return false;
        }
    }
    bool key(const std::string& line, const std::vector<std::string>& args)
    {
        if(args[1] == "list")
        {
            if(args.size() != 2)
            {
                logging::log("ERR","No more arguments needed, use \"help key\" for more info");
                return false;
            }
            else
            {
                auto keys = network::authentication::known_users.get_all();
                for(auto& k: keys)
                    logging::log("MSG","- " HIGHLIGHT +k+ RESET);
                return true;
            }
        }else if(args[1] == "show")
        {
            if(args.size() == 2)
            {
                logging::log("ERR","You must provide the user to get from the list of known users, use \"help key\" for more info");
                return false;
            }
            else if(args.size() == 4)
            {
                logging::log("ERR","No more arguments needed, use \"help key\" for more info");
                return false;
            }
            else
            {
                try
                {
                    auto key = network::authentication::known_users.get_key(args[2]);
                    if(key.length() == 0)
                        logging::log("MSG","User " HIGHLIGHT + args[2] + RESET " is " ERR_TAG "blacklisted for a risk of MiM attack" RESET);
                    else
                        logging::log("MSG","Key: " HIGHLIGHT + key + RESET);
                    return true;
                }
                catch(network::authentication::KnownUsers::KeyNotFound&)
                {
                    logging::log("ERR","User " HIGHLIGHT +args[2]+ RESET " is not in the list of known users, use \"key list\" to get a list of known users");
                    return false;
                }
            }
        }else if(args[1] == "delete")
        {
            if(args.size() == 2)
            {
                logging::log("ERR","You must provide the user to remove from the list of known users, use \"help key\" for more info");
                return false;
            }
            else if(args.size() == 4)
            {
                logging::log("ERR","No more arguments needed, use \"help key\" for more info");
                return false;
            }
            else
            {
                if(network::authentication::known_users.delete_key(args[2]))
                {
                    logging::log("MSG","Key associated with user " HIGHLIGHT +args[2]+ RESET " was successfully removed");
                    return true;
                }
                else
                {
                    logging::log("ERR","User " HIGHLIGHT +args[2]+ RESET " is not in the list of known users, use \"key list\" to get a list of known users");
                    return false;
                }
            }
        }
        else if(args[1] == "add")
        {
            if(args.size() != 4)
            {
                logging::log("ERR","You must provide the user to add to the list of known users and the key to associate to it, use \"help key\" for more info");
                return false;
            }
            else
            {
                if(args[3].find(':') == args[3].npos)
                {
                    logging::log("ERR","The key must be in the format <b64_encoded n>:<b64_encoded e>");
                    return false;
                }
                else if(network::authentication::known_users.add_key(args[2],args[3]))
                {
                    logging::log("MSG","Key successfully associated with user " HIGHLIGHT + args[2] + RESET);
                    return true;
                }
                else
                {
                    logging::log("ERR","User " HIGHLIGHT +args[2]+ RESET " already has a key associated to it, if you want to change it you must first delete it with \"key delete " HIGHLIGHT +parsing::compose_message({args[2]})+ RESET "\"");
                    return false;
                }
            }
        }
        else
        {
            logging::log("ERR","The second argument must be add, delete, show or list, use \"help key\" for more info");
            return false;
        }
    }
}