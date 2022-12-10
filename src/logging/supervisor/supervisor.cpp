#include "supervisor.hpp"
#include "../../defines.hpp"
#include <boost/thread.hpp>
#include "../logging.hpp"
#include "../../multithreading/multithreading.hpp"
#include "../../network/udp/udp.hpp"
#include "../../network/audio/audio.hpp"

namespace logging::supervisor
{
    unsigned int sleep_time;
    void supervisor()
    {
        while(true)
        {
            supervisor_log(network::udp::connection_map.size(),multithreading::get_count(),network::audio::input_dropped_frames,network::audio::output_dropped_frames);
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