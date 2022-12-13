#include "timecheck.hpp"
#include "../../defines.hpp"
#include "../../ansi_escape.hpp"
#include <boost/thread.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_smallint.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../udp/udp.hpp"
#include "../connection/connection.hpp"
#include "../../parsing/parsing.hpp"
#include "../../logging/logging.hpp"
#include "../../multithreading/multithreading.hpp"
namespace network::timecheck
{
    #define PING_EVERY 1 // s
    #define RUN_EVERY 500 // ms
    #define CONFIDENCE 3 // times the avg latency to determine timeout
    #define MAX_STRIKES 3 // subsequent packets lost for disconnect
    #define REQUEST_TIMEOUT 10 // s 
    #define CONNECTION_TIMEOUT 1 // s
    boost::random::mt19937 rng;
    void timecheck()
    {
        while(true)
        {
            auto users = udp::connection_map.get_connected_users();
            // ping check
            for(auto& [name,endpoint]: users)
            {
                auto& data_ref = udp::connection_map[name];
                auto data = data_ref;
                auto now = boost::posix_time::microsec_clock::local_time();
                if(data.last_ping_id == 0) // not waiting for ping
                {
                    if(data.ping_sent + boost::posix_time::seconds(PING_EVERY) < now or data.ping_sent == boost::posix_time::ptime{})
                    {// need to send another ping
                        data_ref.last_ping_id = boost::random::uniform_smallint<unsigned short>(0,65535)(rng);
                        data_ref.ping_sent = boost::posix_time::microsec_clock::local_time();
                        udp::send(parsing::compose_message({"PING",std::to_string(data_ref.last_ping_id)}),endpoint);
                        //logging::log("DBG","Sent ping to " HIGHLIGHT + name + RESET);
                    }
                }else
                {
                    if((now - data.ping_sent) > (data.avg_latency*CONFIDENCE))
                    {
                        data_ref.last_ping_id = 0;
                        data_ref.offline_strikes += 1;
                        if(data_ref.offline_strikes > MAX_STRIKES)
                        {//peer is offline
                            udp::send(parsing::compose_message({"DISCONNECT","connection timed out"}),endpoint);
                            logging::log("ERR","User \"" HIGHLIGHT + name + RESET "\" went offline");
                            udp::connection_map.remove_user(name);
                        }
                    }
                }
                boost::this_thread::interruption_point();
                boost::this_thread::yield();
            }
            {
                // no response check
                std::vector<std::string> failed_requests;
                {
                    std::unique_lock lock_requests(udp::requested_clients_mutex);
                    for(auto& [name, time]: udp::requested_clients)
                    {
                        auto now = boost::posix_time::microsec_clock::local_time();
                        if((now - time).total_seconds() > REQUEST_TIMEOUT)
                            failed_requests.emplace_back(name);
                    }
                }
                for(auto& name: failed_requests)
                {
                    udp::server_request_fail(name);
                    logging::log("ERR","User \"" HIGHLIGHT +name+ RESET "\" timed out");
                    boost::this_thread::interruption_point();
                    boost::this_thread::yield();
                }
            }
            {
                // dead during connection check
                std::vector<boost::asio::ip::udp::endpoint> failed_connections;
                {
                    std::unique_lock lock_connections(connection::status_map_mutex);
                    for(auto& [endpoint, info]: connection::status_map)
                    {
                        auto now = boost::posix_time::microsec_clock::local_time();
                        if((now - info.registration_time).total_seconds() > CONNECTION_TIMEOUT)
                            failed_connections.emplace_back(endpoint);
                    }
                }
                for(auto& endpoint: failed_connections)
                {
                    if(connection::connection_timed_out(endpoint))
                        logging::log("ERR","Connection with " HIGHLIGHT +endpoint.address().to_string()+ RESET ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET);
                    boost::this_thread::interruption_point();
                    boost::this_thread::yield();
                }
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(RUN_EVERY));
        }
    }
    void init()
    {
        multithreading::add_service("timecheck",timecheck);       
    }
}