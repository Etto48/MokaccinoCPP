#include "connection.hpp"
namespace network::connection
{
    MessageQueue connection_queue;
    std::string username;
    struct StatusEntry
    {
        std::string expected_message;
        std::string name;
    };
    std::mutex status_map_mutex;
    std::map<boost::asio::ip::udp::endpoint,StatusEntry> status_map;
    void connection_handler()
    {
        while(true)
        {
            auto item = connection_queue.pull();
            auto args = parsing::msg_split(item.msg);
            if(args.size() > 0)
            {
                std::unique_lock lock(status_map_mutex);
                if(args[0] == "CONNECT" && args.size() == 2)
                {
                    status_map[item.src_endpoint] = {"CONNECTED",args[1]};
                    udp::send(parsing::compose_message({"HANDSHAKE",username}),item.src_endpoint);
                    logging::log("DBG",_format("Handled CONNECT from {}:{}",item.src_endpoint.address().to_string(),item.src_endpoint.port()));
                }
                else if(args[0] == "HANDSHAKE" && args.size() == 2 && status_map[item.src_endpoint].expected_message == "HANDSHAKE" && status_map[item.src_endpoint].name == args[1])
                {
                    status_map.erase(item.src_endpoint);
                    udp::connection_map.add_user(args[1],item.src_endpoint);
                    udp::send(parsing::compose_message({"CONNECTED",username}),item.src_endpoint);
                    logging::log("DBG",_format("Handled HANDSHAKE from {}:{}",item.src_endpoint.address().to_string(),item.src_endpoint.port()));
                }
                else if(args[0] == "CONNECTED" && args.size() == 2 && status_map[item.src_endpoint].expected_message == "CONNECTED" && status_map[item.src_endpoint].name == args[1])
                {
                    status_map.erase(item.src_endpoint);
                    udp::connection_map.add_user(args[1],item.src_endpoint);
                    logging::log("DBG",_format("Handled CONNECTED from {}:{}",item.src_endpoint.address().to_string(),item.src_endpoint.port()));
                }
                //REQUEST <to>
                else if(args[0] == "REQUEST" && args.size() == 2)
                {
                    if(!udp::connection_map.check_user(args[1]))
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
                else if(args[0] == "REQUESTED" && args.size() == 3 && !udp::connection_map.check_user(args[1]) && udp::connection_map.server() == item.src_endpoint)
                {
                    
                    try{
                        auto endpoint = parsing::endpoint_from_str(args[2]);
                        status_map[endpoint] = {"HANDSHAKE",args[1]};
                        udp::send(parsing::compose_message({"CONNECT",username}),endpoint);
                    }catch(parsing::EndpointFromStrError&)
                    {}
                }
                #ifdef _DEBUG
                else if(args[0] == "TEST")
                {
                    logging::connection_test_log(item);
                }
                #endif
                else {
                    logging::log("DBG",_format("Dropped {} from {}:{}",args[0],item.src_endpoint.address().to_string(),item.src_endpoint.port()));
                }
            }
        }
    }
    void init(std::string local_username)
    {
        username = local_username;
        udp::register_queue("CONNECT",connection_queue,false);
        udp::register_queue("HANDSHAKE",connection_queue,false);
        udp::register_queue("CONNECTED",connection_queue,false);
        udp::register_queue("REQUEST",connection_queue,true);
        udp::register_queue("REQUESTED",connection_queue,true);
        #ifdef _DEBUG
        udp::register_queue("TEST",connection_queue,true);
        #endif
        multithreading::add_service("connection_handler",connection_handler);
    }
    void connect(const boost::asio::ip::udp::endpoint& endpoint,const std::string& expected_name)
    {
        std::unique_lock lock(status_map_mutex);
        status_map[endpoint] = {"HANDSHAKE",expected_name};
        udp::send(parsing::compose_message({"CONNECT",username}),endpoint);
    }

}