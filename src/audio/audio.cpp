#include "audio.hpp"
#include <portaudio.h>
namespace audio
{
    network::MessageQueue audio_queue;
    std::mutex name_mutex;
    std::string pending_name;
    std::string audio_name;
    
    void audio()
    {
        while(true)
        {
            auto item = audio_queue.pull();
            auto args = parsing::msg_split(item.msg);
            std::unique_lock lock(name_mutex);
            if(args.size() == 1 && args[0] == "AUDIOSTART")
            {
                if(audio_name.length() == 0)
                {//no user connected for voice
                    network::udp::send(parsing::compose_message({"AUDIOACCEPT"}),item.src_endpoint);
                    //start sending AUDIO
                }else
                {//user already connected for voice
                    network::udp::send(parsing::compose_message({"AUDIOSTOP"}),item.src_endpoint);
                }
            }else if(args.size() == 1 && args[0] == "AUDIOACCEPT" && pending_name == item.src && audio_name.length()==0)
            {
                audio_name = item.src;
                pending_name = "";

                //start sending AUDIO
            }else if(args.size() == 1 && args[0] == "AUDIOSTOP" && (audio_name == item.src || pending_name == item.src))
            {
                audio_name = "";
                pending_name = "";
            }else if(args.size() == 2 && args[0] == "AUDIO" && audio_name == item.src)
            {

            }else
            {
                logging::log("DBG","Dropped "+ args[0] +" from "+ item.src_endpoint.address().to_string() +":"+ std::to_string(item.src_endpoint.port()));
            }
        }
    }
    void init()
    {
        network::udp::register_queue("AUDIOSTART",audio_queue,true);
        network::udp::register_queue("AUDIOACCEPT",audio_queue,true);
        network::udp::register_queue("AUDIOSTOP",audio_queue,true);
        network::udp::register_queue("AUDIO",audio_queue,true);
        multithreading::add_service("audio",audio);
    }
    void start_call(const std::string& name)
    {
        std::unique_lock lock(name_mutex);
        if(audio_name.length() == 0)
        {
            pending_name = name;
            network::udp::send(parsing::compose_message({"AUDIOSTART"}),name);
        }
    }
    void stop_call()
    {
        std::unique_lock lock(name_mutex);
        if(audio_name.length() != 0)
        {
            audio_name = "";
            network::udp::send(parsing::compose_message({"AUDIOSTOP"}),audio_name);
        }
    }
}