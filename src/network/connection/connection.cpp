#include "connection.hpp"
#include "../../defines.hpp"
#include "../../ansi_escape.hpp"
#include <map>
#include "../../multithreading/multithreading.hpp"
#include "../../parsing/parsing.hpp"
#include "../../logging/logging.hpp"
#include "../../terminal/terminal.hpp"
#include "../udp/udp.hpp"
#include "../MessageQueue/MessageQueue.hpp"
namespace network::connection
{
    MessageQueue connection_queue;
    std::string username;
    std::vector<std::string> whitelist;
    struct StatusEntry
    {
        std::string expected_message;
        std::string name;
    };
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
        status_map[endpoint] = {"CONNECTED",name};
        udp::send(parsing::compose_message({"HANDSHAKE",username}),endpoint);
        logging::log("DBG","Handled CONNECT from "+endpoint.address().to_string()+":"+std::to_string(endpoint.port()));
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
                    if(check_whitelist(args[1]))
                    {
                        accept_connection(item.src_endpoint,args[1]);
                    }
                    else
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
                    logging::log("DBG","Handled HANDSHAKE from "+item.src_endpoint.address().to_string()+":"+std::to_string(item.src_endpoint.port()));
                }
                else if(args[0] == "CONNECTED" and args.size() == 2 and status_map[item.src_endpoint].expected_message == "CONNECTED" and status_map[item.src_endpoint].name == args[1])
                {
                    status_map.erase(item.src_endpoint);
                    udp::connection_map.add_user(args[1],item.src_endpoint);
                    logging::log("DBG","Handled CONNECTED from "+item.src_endpoint.address().to_string()+":"+std::to_string(item.src_endpoint.port()));
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
                        status_map[endpoint] = {"HANDSHAKE",args[1]};
                        udp::send(parsing::compose_message({"CONNECT",username}),endpoint);
                    }catch(parsing::EndpointFromStrError&)
                    {}
                }
                else if(args[0] == "FAIL" and args.size() == 2)
                {
                    //TODO: handle not found
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
                    logging::log("DBG","Dropped "+ args[0] +" from "+ item.src_endpoint.address().to_string() +":"+ std::to_string(item.src_endpoint.port()));
                }
            }
        }
    }
    void init(std::string local_username, const std::vector<std::string>& whitelist)
    {
        username = local_username;
        network::connection::whitelist = whitelist;
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
        status_map[endpoint] = {"HANDSHAKE",expected_name};
        udp::send(parsing::compose_message({"CONNECT",username}),endpoint);
    }

}