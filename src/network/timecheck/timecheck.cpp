#include "timecheck.hpp"
#include "../../defines.hpp"
#include "../../ansi_escape.hpp"
#include <boost/thread.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_smallint.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../udp/udp.hpp"
#include "../../parsing/parsing.hpp"
#include "../../logging/logging.hpp"
#include "../../multithreading/multithreading.hpp"
namespace network::timecheck
{
    #define PING_EVERY 1
    #define RUN_EVERY 500
    #define CONFIDENCE 3
    #define MAX_STRIKES 3 // subsequent packets lost for disconnect
    boost::random::mt19937 rng;
    void timecheck()
    {
        while(true)
        {
            auto users = udp::connection_map.get_connected_users();
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
                boost::this_thread::yield();
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(RUN_EVERY));
        }
    }
    void init()
    {
        multithreading::add_service("timecheck",timecheck);       
    }
}