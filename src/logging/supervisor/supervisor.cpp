#include "supervisor.hpp"
namespace logging::supervisor
{
    unsigned int sleep_time;
    void supervisor()
    {
        while(true)
        {
            supervisor_log(network::udp::connection_map.size(),multithreading::get_count());
            boost::this_thread::sleep_for(boost::chrono::seconds(sleep_time));
        }
    }
    void init(unsigned int timeout_seconds)
    {
        if(DEBUG)
        {
            sleep_time = timeout_seconds;
            multithreading::add_service("supervisor",supervisor);
        }
    }
}