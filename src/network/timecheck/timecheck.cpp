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
    #define MAX_STRIKES 5 // subsequent packets lost for disconnect
    #define REQUEST_TIMEOUT 10 // s 
    #define CONNECTION_TIMEOUT 10 // s
    #define ENCRYPTION_TIMEOUT 1 // s
    constexpr boost::posix_time::time_duration MIN_PING(boost::posix_time::millisec(20)); // ms
    boost::random::mt19937 rng;
    void timecheck()
    {
        while(true)
        {
            auto users = udp::connection_map.get_connected_users();
            // ping check
            for(auto& [name,endpoint]: users)
            {
                std::unique_lock lock(udp::connection_map.obj);
                auto& data_ref = udp::connection_map[name];
                auto now = boost::posix_time::microsec_clock::local_time();
                if(data_ref.last_ping_id == 0) // not waiting for ping
                {
                    if(data_ref.ping_sent + boost::posix_time::seconds(PING_EVERY) < now or data_ref.ping_sent == boost::posix_time::ptime{})
                    {// need to send another ping
                        data_ref.last_ping_id = boost::random::uniform_smallint<unsigned short>(0,65535)(rng);
                        data_ref.ping_sent = boost::posix_time::microsec_clock::local_time();
                        udp::send(parsing::compose_message({"PING",std::to_string(data_ref.last_ping_id)}),endpoint);
                        //logging::log("DBG","Sent ping to " HIGHLIGHT + name + RESET);
                    }
                }else
                {
                    auto latency = data_ref.avg_latency > MIN_PING ? data_ref.avg_latency : MIN_PING;
                    if((now - data_ref.ping_sent) > (latency*CONFIDENCE))
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
                if(not data_ref.encrypted and data_ref.asymmetric_key.length() > 0 and (now - data_ref.crypt_requested).total_seconds() > ENCRYPTION_TIMEOUT)
                {
                    if(not data_ref.symmetric_key_valid)
                    {// sent CRYPTSTART and waiting for CRYPTACCEPT
                        auto key = udp::crypto::string_to_privkey(data_ref.asymmetric_key);
                        auto m = parsing::compose_message({"CRYPTSTART",udp::crypto::pubkey_to_string(key.get())});
                        parsing::sign_and_append(m);
                        data_ref.crypt_requested = now;
                        udp::send(m,data_ref.endpoint);
                    }
                    else
                    {// sent CRYPTACCEPT and waiting for CRYPTACK
                        auto key = udp::crypto::string_to_privkey(data_ref.asymmetric_key);
                        auto m = parsing::compose_message({"CRYPTACCEPT",udp::crypto::pubkey_to_string(key.get())});
                        parsing::sign_and_append(m);
                        data_ref.crypt_requested = now;
                        udp::send(m,data_ref.endpoint); 
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
                        logging::log("ERR","Connection with " HIGHLIGHT +endpoint.address().to_string()+ RESET ":" HIGHLIGHT + std::to_string(endpoint.port()) + RESET + " timed out");
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