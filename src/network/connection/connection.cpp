#include "connection.hpp"
#include "../../defines.hpp"
#include "../../ansi_escape.hpp"
#include <map>
#include "../../multithreading/multithreading.hpp"
#include "../../parsing/parsing.hpp"
#include "../../logging/logging.hpp"
#include "../../terminal/terminal.hpp"
#include "../udp/udp.hpp"
#include "../ConnectionAction/ConnectionAction.hpp"
#include "../MessageQueue/MessageQueue.hpp"
#include "../authentication/authentication.hpp"
#include <openssl/evp.h>
namespace network::connection
{
    MessageQueue connection_queue;
    std::string username;
    ConnectionAction default_action = ConnectionAction::PROMPT;
    std::vector<std::string> whitelist;
    std::mutex status_map_mutex;
    std::map<boost::asio::ip::udp::endpoint,StatusEntry> status_map;
    bool autoencryption = false;
    bool check_whitelist(const std::string& name)
    {
        for(auto& a: whitelist)
        {
            if(a==name)
                return true;
        }
        return false;
    }
    void start_encryption(const std::string& name)
    {
        try
        {
            std::unique_lock lock(udp::connection_map.obj);
            auto& info = udp::connection_map[name];
            auto key = udp::crypto::gen_ecdhe_key();
            info.asymmetric_key = udp::crypto::privkey_to_string(key.get());
            auto m = parsing::compose_message({"CRYPTSTART",udp::crypto::pubkey_to_string(key.get())});
            parsing::sign_and_append(m);
            udp::send(m,info.endpoint);
        }catch(DataMap::NotFound&)
        {}
    }
    void stop_encryption(const std::string& name)
    {
        try
        {
            std::unique_lock lock(udp::connection_map.obj);
            auto& info = udp::connection_map[name];
            info.encrypted = false;
            info.asymmetric_key = "";
            udp::send(parsing::compose_message({"CRYPTSTOP"}),info.endpoint);
        }catch(DataMap::NotFound&)
        {}
    }
    void accept_connection(const boost::asio::ip::udp::endpoint& endpoint, const std::string& name,const std::string& received_nonce, const std::string& received_public_key, const std::string& sm)
    {
        authentication::known_users.add_key(name,received_public_key);
        if(not parsing::verify_signature_from_message(sm,name))
        {
            udp::send(parsing::compose_message({"DISCONNECT","authentication error"}),endpoint);
            logging::log("ERR","Connection refused from \"" HIGHLIGHT + name + RESET "\" because of an authentication error");    
        }else
        {
            auto nonce = authentication::generate_nonce();
            status_map[endpoint] = {"CONNECTED",name,nonce,received_nonce,boost::posix_time::microsec_clock::local_time()};
            auto m = parsing::compose_message({"HANDSHAKE",username,nonce,received_nonce,authentication::local_public_key()});
            parsing::sign_and_append(m);
            udp::send(m,endpoint);
            logging::log("MSG","Connection accepted from " HIGHLIGHT + name + RESET " at " HIGHLIGHT + endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET);
        }
    }
    void accept_server_request(const boost::asio::ip::udp::endpoint& endpoint, const std::string& name, const std::string& received_public_key)
    {
        authentication::known_users.add_key(name,received_public_key);
        auto nonce = authentication::generate_nonce();
        status_map[endpoint] = {"HANDSHAKE",name,nonce,"",boost::posix_time::microsec_clock::local_time()};
        auto m = parsing::compose_message({"CONNECT",username,nonce,authentication::local_public_key()});
        parsing::sign_and_append(m);
        udp::send(m,endpoint);
        logging::log("MSG","Connection accepted from " HIGHLIGHT + name + RESET " at " HIGHLIGHT + endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET);
    }
    void connection()
    {
        while(true)
        {
            auto item = connection_queue.pull();
            auto args = parsing::msg_split(item.msg);
            if(args.size() > 0)
            {
                std::unique_lock lock(status_map_mutex);
                if(args[0] == "CONNECT" and args.size() == 5 and not udp::connection_map.check_user(args[1]))
                {
                    if(check_whitelist(args[1]) or default_action == ConnectionAction::ACCEPT)
                    {
                        accept_connection(item.src_endpoint,args[1],args[2],args[3],item.msg);
                    }else if(default_action == ConnectionAction::REFUSE)
                    {
                        udp::send(parsing::compose_message({"DISCONNECT","connection refused"}),item.src_endpoint);
                        logging::log("MSG","Connection refused automatically from \"" HIGHLIGHT +args[1]+ RESET "\"");
                    }else
                        terminal::input(
                            "User \"" HIGHLIGHT + args[1] + RESET 
                            "\" (" HIGHLIGHT +item.src_endpoint.address().to_string()+ RESET 
                            ":" HIGHLIGHT + std::to_string(item.src_endpoint.port()) + RESET 
                            ") requested to connect, accept? (y/n)",
                            [args,item](const std::string& input){
                                if(input == "Y" or input == "y")
                                    accept_connection(item.src_endpoint,args[1],args[2],args[3],item.msg);
                                else
                                {
                                    udp::send(parsing::compose_message({"DISCONNECT","connection refused"}),item.src_endpoint);
                                    logging::log("MSG","Connection refused from \"" HIGHLIGHT +args[1]+ RESET "\"");
                                }
                            });
                }
                else if(args[0] == "HANDSHAKE" and args.size() == 6 and status_map[item.src_endpoint].expected_message == "HANDSHAKE")
                {
                    authentication::known_users.add_key(args[1],args[4]);
                    auto sent_nonce = status_map[item.src_endpoint].sent_nonce;
                    if(parsing::verify_signature_from_message(item.msg,args[1]) and sent_nonce == args[3])
                    {
                        status_map.erase(item.src_endpoint);
                        udp::connection_map.add_user(args[1],item.src_endpoint);
                        auto m = parsing::compose_message({"CONNECTED",args[2]});
                        parsing::sign_and_append(m);
                        udp::send(m,item.src_endpoint);
                        if(not udp::server_request_success(args[1]))
                            logging::log("MSG","Connection accepted from " HIGHLIGHT + args[1] + RESET " at " HIGHLIGHT + item.src_endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(item.src_endpoint.port()) + RESET);
                    }
                    else
                    {
                        logging::log("ERR","Connection refused from \"" HIGHLIGHT + args[1] + RESET "\" because of an authentication error");
                        status_map.erase(item.src_endpoint);
                    }
                }
                else if(args[0] == "CONNECTED" and args.size() == 3 and status_map[item.src_endpoint].expected_message == "CONNECTED")
                {
                    auto name = status_map[item.src_endpoint].name;
                    auto sent_nonce = status_map[item.src_endpoint].sent_nonce;
                    if(parsing::verify_signature_from_message(item.msg,name) and sent_nonce == args[1])
                    {
                        status_map.erase(item.src_endpoint);
                        udp::connection_map.add_user(name,item.src_endpoint);
                        logging::log("MSG","Peer " HIGHLIGHT + item.src_endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(item.src_endpoint.port()) + RESET " is now connected as user \"" HIGHLIGHT+name+RESET"\"");
                        if(autoencryption)
                            start_encryption(item.src);
                    }
                    else
                    {
                        logging::log("ERR","Connection refused from \"" HIGHLIGHT + name + RESET "\" because of an authentication error");
                        status_map.erase(item.src_endpoint);
                    }
                }
                else if(args[0] == "DISCONNECT" and args.size() >= 1 and args.size() <= 2)
                {
                    std::string reason;
                    if(args.size() == 2)
                        reason = args[1];
                    logging::received_disconnect_log(item.src,reason);
                    udp::connection_map.remove_user(item.src);
                }
                //REQUEST <to>
                else if(args[0] == "REQUEST" and args.size() == 2)
                {
                    try{
                        auto key = authentication::known_users.get_key(args[1]);
                        auto m = parsing::compose_message({"KEY",args[1],key});
                        parsing::sign_and_append(m);
                        udp::send(m,item.src_endpoint);
                    }catch(authentication::KnownUsers::KeyNotFound&)
                    {}
                    if(not udp::connection_map.check_user(args[1]))
                    {//not found
                        udp::send(parsing::compose_message({"FAIL",args[1]}),item.src_endpoint);
                    }
                    else
                    {
                        std::unique_lock lock(udp::connection_map.obj);
                        auto& target = udp::connection_map[args[0]];
                        auto m = parsing::compose_message(
                            {"REQUESTED",
                            item.src,
                            item.src_endpoint.address().to_string()+':'+std::to_string(item.src_endpoint.port())});
                        parsing::sign_and_append(m);
                        udp::send(m,
                            target.endpoint);
                    }
                }
                else if(args[0] == "KEY" and args.size() == 4 and udp::connection_map.server() == item.src_endpoint)
                {
                    if(parsing::verify_signature_from_message(item.msg,item.src))
                        authentication::known_users.add_key(args[1],args[2]);
                    else
                    {
                        logging::log("ERR","Certificate sent from " HIGHLIGHT +item.src+ RESET " for the user " HIGHLIGHT + args[1] + RESET " was not valid, if you want to retry the connection (at your own risk) you can use the command \"key delete " HIGHLIGHT +parsing::compose_message({args[1]})+ RESET "\" and then retry to connect");
                        //we disable the connection with the user because there is a risk of MiM attack
                        authentication::known_users.replace_key(args[1],"");
                    }
                }
                //REQUESTED <from> <at>
                else if(args[0] == "REQUESTED" and args.size() == 5 and not udp::connection_map.check_user(args[1]) and udp::connection_map.server() == item.src_endpoint)
                {
                    if(parsing::verify_signature_from_message(item.msg,item.src))
                    {
                        try{
                            auto endpoint = parsing::endpoint_from_str(args[2]);
                            if(check_whitelist(args[1]) or default_action == ConnectionAction::ACCEPT)
                            {
                                accept_server_request(endpoint,args[1],args[3]);
                            }else if(default_action == ConnectionAction::REFUSE)
                            {
                                logging::log("MSG","Connection refused automatically from \"" HIGHLIGHT +args[1]+ RESET "\"");
                            }else
                                terminal::input(
                                    "User \"" HIGHLIGHT + args[1] + RESET 
                                    "\" (" HIGHLIGHT +endpoint.address().to_string()+ RESET 
                                    ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET 
                                    ") requested to connect, accept? (y/n)",
                                    [args,item,endpoint](const std::string& input){
                                        if(input == "Y" or input == "y")
                                            accept_server_request(endpoint,args[1],args[3]);
                                        else
                                        {
                                            logging::log("MSG","Connection refused from \"" HIGHLIGHT +args[1]+ RESET "\"");
                                        }
                                });
                        }catch(parsing::EndpointFromStrError&)
                        {}
                    }
                    else
                    {
                        logging::log("ERR","User " HIGHLIGHT +args[1]+ RESET " requested you at " HIGHLIGHT +item.src+ RESET "but its key was modified, for safety reasons the connection was interrupted");
                    }
                }
                else if(args[0] == "FAIL" and args.size() == 2)
                {
                    udp::server_request_fail(args[1]);
                    logging::log("ERR","User \"" HIGHLIGHT + args[1] + RESET "\" not found at " HIGHLIGHT + item.src + RESET);
                }
                else if(DEBUG and args[0] == "TEST")
                {
                    logging::connection_test_log(item);
                }
                else if(args[0] == "PING" and args.size() == 2)
                {
                    udp::send(parsing::compose_message({"PONG",args[1]}),item.src_endpoint);
                    //logging::log("DBG","Handled ping from " HIGHLIGHT + item.src + RESET);
                }
                else if(args[0] == "PONG" and args.size() == 2)
                {
                    std::unique_lock lock(udp::connection_map.obj);
                    auto now = boost::posix_time::microsec_clock::local_time();
                    auto& data = udp::connection_map[item.src];
                    if(std::to_string(data.last_ping_id) == args[1])
                    {//it's the last ping
                        data.last_ping_id = 0; // ping received
                        data.last_latency = now - data.ping_sent;
                        data.offline_strikes = 0; // no strikes, just packet lost
                        #define EXP_AVG_FACTOR 8
                        #define EXP_AVG_MAX    10
                        if(data.avg_latency == boost::posix_time::time_duration{})
                            data.avg_latency = data.last_latency;
                        else
                            data.avg_latency = (data.avg_latency * EXP_AVG_FACTOR / EXP_AVG_MAX) + (data.last_latency * (EXP_AVG_MAX-EXP_AVG_FACTOR) / EXP_AVG_MAX);
                        //logging::log("DBG","Handled pong from " HIGHLIGHT + item.src + RESET);
                    }
                }
                // CRYPTSTART <one time public key> <signature>
                else if(args[0] == "CRYPTSTART" and args.size() == 3)
                {
                    if(parsing::verify_signature_from_message(item.msg,item.src))
                    {
                        try{
                            std::unique_lock lock(udp::connection_map.obj);
                            auto& user_info = udp::connection_map[item.src];
                            auto key = udp::crypto::gen_ecdhe_key();
                            try
                            {
                                auto remote_key = udp::crypto::string_to_pubkey(args[1]);
                                user_info.asymmetric_key = udp::crypto::privkey_to_string(key.get());
                                user_info.symmetric_key = udp::crypto::ecdhe(key.get(),remote_key.get());
                                auto m = parsing::compose_message({"CRYPTACCEPT",udp::crypto::pubkey_to_string(key.get())});
                                parsing::sign_and_append(m);
                                udp::send(m,item.src_endpoint);
                                user_info.encrypted = true;
                                logging::log("MSG","Connection with " HIGHLIGHT +item.src+ RESET " is now encrypted");
                            }catch(std::runtime_error&)
                            {}
                        }catch(DataMap::NotFound&)
                        {}
                    }
                    else
                    {// maybe MiM
                        stop_encryption(item.src);
                    }
                }
                // CRYPTACCEPT <one time public key> <signature>
                else if(args[0] == "CRYPTACCEPT" and args.size() == 3)
                {
                    if(parsing::verify_signature_from_message(item.msg,item.src))
                    {
                        try
                        {
                            std::unique_lock lock(udp::connection_map.obj);
                            auto& user_info = udp::connection_map[item.src];
                            if(user_info.asymmetric_key.length() != 0)
                            {
                                auto key = udp::crypto::string_to_privkey(user_info.asymmetric_key);
                                try
                                {
                                    auto remote_key = udp::crypto::string_to_pubkey(args[1]);
                                    user_info.symmetric_key = udp::crypto::ecdhe(key.get(),remote_key.get());
                                    user_info.encrypted = true;
                                    logging::log("MSG","Connection with " HIGHLIGHT +item.src+ RESET " is now encrypted");
                                }catch(std::runtime_error&)
                                {}
                            }
                        }
                        catch(DataMap::NotFound&)
                        {}
                    }
                    else
                    {
                        stop_encryption(item.src);
                    }
                }
                // CRYPTSTOP
                else if(args[0] == "CRYPTSTOP" and args.size() == 1)
                {
                    try
                    {
                        std::unique_lock lock(udp::connection_map.obj);
                        auto& user_info = udp::connection_map[item.src];
                        if(user_info.asymmetric_key.length() != 0 and not user_info.encrypted)
                            logging::log("ERR","Encryption refused from " HIGHLIGHT +item.src+ RESET); 
                        else if(user_info.encrypted)       
                            logging::log("MSG","User " HIGHLIGHT +item.src+ RESET " stopped the encryption");    
                        user_info.asymmetric_key = "";
                        user_info.encrypted = false;
                    }
                    catch(DataMap::NotFound&)
                    {}
                }
                else {
                    logging::log("DBG","Dropped " HIGHLIGHT + args[0] + RESET " from " HIGHLIGHT + item.src_endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(item.src_endpoint.port()) + RESET);
                }
            }
        }
    }
    void init(std::string local_username, const std::vector<std::string>& whitelist, const std::string& default_action, bool encrypt_by_default)
    {
        username = local_username;
        autoencryption = encrypt_by_default;
        logging::log("DBG","Username set as \"" HIGHLIGHT + username + RESET "\"");
        network::connection::whitelist = whitelist;
        if(default_action == "accept")
        {
            network::connection::default_action = ConnectionAction::ACCEPT;
        }else if(default_action == "refuse")
        {
            network::connection::default_action = ConnectionAction::REFUSE;
        }else if(default_action == "prompt")
        {
            network::connection::default_action = ConnectionAction::PROMPT;
        }
        else
        {
            network::connection::default_action = ConnectionAction::PROMPT;
            logging::log("ERR","Error in configuration file at network.connection.default_action: this must be one of \"accept\", \"refuse\" or \"prompt\", \"prompt\" was selected as default");
        }
        
        udp::register_queue("CONNECT",connection_queue,false);
        udp::register_queue("HANDSHAKE",connection_queue,false);
        udp::register_queue("CONNECTED",connection_queue,false);
        udp::register_queue("DISCONNECT",connection_queue,true);
        udp::register_queue("REQUEST",connection_queue,true);
        udp::register_queue("KEY",connection_queue,true);
        udp::register_queue("REQUESTED",connection_queue,true);
        udp::register_queue("FAIL",connection_queue,true);
        udp::register_queue("PING",connection_queue,true);
        udp::register_queue("PONG",connection_queue,true);
        udp::register_queue("CRYPTSTART",connection_queue,true);
        udp::register_queue("CRYPTACCEPT",connection_queue,true);
        udp::register_queue("CRYPTSTOP",connection_queue,true);
        if(DEBUG)
            udp::register_queue("TEST",connection_queue,true);
        multithreading::add_service("connection",connection);
    }
    bool connect(const boost::asio::ip::udp::endpoint& endpoint,const std::string& expected_name)
    {
        try{//already connected
            std::unique_lock lock(udp::connection_map.obj);
            auto name = udp::connection_map[endpoint].name;
            logging::log("ERR","You are already connected with " HIGHLIGHT +name+ RESET " at " HIGHLIGHT + endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET);
            return false;
        }catch(DataMap::NotFound&)
        {
            std::unique_lock lock(status_map_mutex);
            auto nonce = authentication::generate_nonce();
            status_map[endpoint] = {"HANDSHAKE",expected_name,nonce,"",boost::posix_time::microsec_clock::local_time()};
            auto m = parsing::compose_message({"CONNECT",username,nonce,authentication::local_public_key()});
            parsing::sign_and_append(m);
            udp::send(m,endpoint);
            return true;
        }
    }
    bool connection_timed_out(const boost::asio::ip::udp::endpoint& endpoint)
    {
        std::unique_lock lock(status_map_mutex);
        auto iter = status_map.find(endpoint);
        if(iter != status_map.end())
        {
            status_map.erase(endpoint);
            return true;
        }
        return false;
    }
}