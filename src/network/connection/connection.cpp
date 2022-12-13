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
namespace network::connection
{
    MessageQueue connection_queue;
    std::string username;
    ConnectionAction default_action = ConnectionAction::PROMPT;
    std::vector<std::string> whitelist;
    std::mutex status_map_mutex;
    std::map<boost::asio::ip::udp::endpoint,StatusEntry> status_map;
    bool check_whitelist(const std::string& name)
    {
        for(auto& a: whitelist)
        {
            if(a==name)
                return true;
        }
        return false;
    }
    void accept_connection(const boost::asio::ip::udp::endpoint& endpoint, const std::string& name)
    {
        status_map[endpoint] = {"CONNECTED",name,boost::posix_time::microsec_clock::local_time()};
        udp::send(parsing::compose_message({"HANDSHAKE",username}),endpoint);
        logging::log("MSG","Connection accepted from " HIGHLIGHT + name + RESET " at " HIGHLIGHT + endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET);
    }
    void accept_server_request(const boost::asio::ip::udp::endpoint& endpoint, const std::string& name)
    {
        status_map[endpoint] = {"HANDSHAKE",name,boost::posix_time::microsec_clock::local_time()};
        udp::send(parsing::compose_message({"CONNECT",username}),endpoint);
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
                if(args[0] == "CONNECT" and args.size() == 2)
                {
                    if(check_whitelist(args[1]) or default_action == ConnectionAction::ACCEPT)
                    {
                        accept_connection(item.src_endpoint,args[1]);
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
                                    accept_connection(item.src_endpoint,args[1]);
                                else
                                {
                                    udp::send(parsing::compose_message({"DISCONNECT","connection refused"}),item.src_endpoint);
                                    logging::log("MSG","Connection refused from \"" HIGHLIGHT +args[1]+ RESET "\"");
                                }
                            });
                }
                else if(args[0] == "HANDSHAKE" and args.size() == 2 and status_map[item.src_endpoint].expected_message == "HANDSHAKE" and status_map[item.src_endpoint].name == args[1])
                {
                    status_map.erase(item.src_endpoint);
                    udp::connection_map.add_user(args[1],item.src_endpoint);
                    udp::send(parsing::compose_message({"CONNECTED",username}),item.src_endpoint);
                    if(not udp::server_request_success(args[1]))
                        logging::log("MSG","Connection accepted from " HIGHLIGHT + args[1] + RESET " at " HIGHLIGHT + item.src_endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(item.src_endpoint.port()) + RESET);
                }
                else if(args[0] == "CONNECTED" and args.size() == 2 and status_map[item.src_endpoint].expected_message == "CONNECTED" and status_map[item.src_endpoint].name == args[1])
                {
                    status_map.erase(item.src_endpoint);
                    udp::connection_map.add_user(args[1],item.src_endpoint);
                    logging::log("MSG","Peer " HIGHLIGHT + item.src_endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(item.src_endpoint.port()) + RESET " is now connected as user \"" HIGHLIGHT+args[1]+RESET"\"");
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
                    if(not udp::connection_map.check_user(args[1]))
                    {//not found
                        udp::send(parsing::compose_message({"FAIL",args[1]}),item.src_endpoint);
                    }
                    else
                    {
                        auto target = udp::connection_map[args[0]];
                        udp::send(parsing::compose_message(
                            {"REQUESTED",
                            item.src,
                            item.src_endpoint.address().to_string()+':'+std::to_string(item.src_endpoint.port())}),
                            target.endpoint);
                    }
                }
                //REQUESTED <from> <at>
                else if(args[0] == "REQUESTED" and args.size() == 3 and not udp::connection_map.check_user(args[1]) and udp::connection_map.server() == item.src_endpoint)
                {
                    try{
                        auto endpoint = parsing::endpoint_from_str(args[2]);
                        if(check_whitelist(args[1]) or default_action == ConnectionAction::ACCEPT)
                        {
                            accept_server_request(item.src_endpoint,args[1]);
                        }else if(default_action == ConnectionAction::REFUSE)
                        {
                            logging::log("MSG","Connection refused automatically from \"" HIGHLIGHT +args[1]+ RESET "\"");
                        }else
                            terminal::input(
                                "User \"" HIGHLIGHT + args[1] + RESET 
                                "\" (" HIGHLIGHT +item.src_endpoint.address().to_string()+ RESET 
                                ":" HIGHLIGHT + std::to_string(item.src_endpoint.port()) + RESET 
                                ") requested to connect, accept? (y/n)",
                                [args,item](const std::string& input){
                                    if(input == "Y" or input == "y")
                                        accept_server_request(item.src_endpoint,args[1]);
                                    else
                                    {
                                        logging::log("MSG","Connection refused from \"" HIGHLIGHT +args[1]+ RESET "\"");
                                    }
                            });
                    }catch(parsing::EndpointFromStrError&)
                    {}
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
                    auto now = boost::posix_time::microsec_clock::local_time();
                    auto& data = udp::connection_map[item.src];
                    if(std::to_string(data.last_ping_id) == args[1])
                    {//it's the last ping
                        data.last_ping_id = 0; // ping received
                        data.last_latency = now - data.ping_sent;
                        data.offline_strikes = 0; // no strikes, just packet lost
                        #define MOVING_AVG_FACTOR 7
                        if(data.avg_latency == boost::posix_time::time_duration{})
                            data.avg_latency = data.last_latency;
                        else
                            data.avg_latency = (data.avg_latency * MOVING_AVG_FACTOR / 10) + (data.last_latency * (10-MOVING_AVG_FACTOR) / 10);
                        //logging::log("DBG","Handled pong from " HIGHLIGHT + item.src + RESET);
                    }
                }
                else {
                    logging::log("DBG","Dropped " HIGHLIGHT + args[0] + RESET " from " HIGHLIGHT + item.src_endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(item.src_endpoint.port()) + RESET);
                }
            }
        }
    }
    void init(std::string local_username, const std::vector<std::string>& whitelist, const std::string& default_action)
    {
        username = local_username;
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
        udp::register_queue("REQUESTED",connection_queue,true);
        udp::register_queue("FAIL",connection_queue,true);
        udp::register_queue("PING",connection_queue,true);
        udp::register_queue("PONG",connection_queue,true);
        if(DEBUG)
            udp::register_queue("TEST",connection_queue,true);
        multithreading::add_service("connection",connection);
    }
    void connect(const boost::asio::ip::udp::endpoint& endpoint,const std::string& expected_name)
    {
        std::unique_lock lock(status_map_mutex);
        status_map[endpoint] = {"HANDSHAKE",expected_name,boost::posix_time::microsec_clock::local_time()};
        udp::send(parsing::compose_message({"CONNECT",username}),endpoint);
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