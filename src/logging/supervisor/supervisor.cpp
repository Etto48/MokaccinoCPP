#include "supervisor.hpp"
#include "../../defines.hpp"
#include "../../ansi_escape.hpp"
#include <boost/thread.hpp>
#include "../logging.hpp"
#include "../../multithreading/multithreading.hpp"
#include "../../network/udp/udp.hpp"
#include "../../network/audio/audio.hpp"
#include "../../network/file/file.hpp"

namespace logging::supervisor
{
    unsigned int sleep_time;
    void supervisor()
    {
        while(true)
        {
            log("DBG", 
                "connections:" HIGHLIGHT + std::to_string(network::udp::connection_map.size()) + RESET " "
                "services:" HIGHLIGHT + std::to_string(multithreading::get_count()) + RESET " "
                "audio_dropped_frames: (I:" HIGHLIGHT + std::to_string(network::audio::input_dropped_frames) + RESET ", O:" HIGHLIGHT + std::to_string(network::audio::output_dropped_frames) + RESET ") "
                "file_transfers: " HIGHLIGHT + std::to_string(network::file::ongoing_transfers_count()) + RESET);
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