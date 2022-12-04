#include "audio.hpp"
#include "../defines.hpp"
#include <mutex>
#include <string_view>
#include <boost/thread/sync_bounded_queue.hpp>
#include <portaudio.h>
#include "../logging/logging.hpp"
#include "../multithreading/multithreading.hpp"
#include "../network/MessageQueue/MessageQueue.hpp"
#include "../network/udp/udp.hpp"
#include "../parsing/parsing.hpp"
#include "../base64/base64.h"
namespace audio
{
    network::MessageQueue audio_queue;
    std::mutex name_mutex;
    std::string pending_name;
    struct AudioBuddy
    {
        std::string name = "";
        boost::asio::ip::udp::endpoint endpoint;
    };

    AudioBuddy audio_buddy;

    PaStream* input_stream;
    PaStream* output_stream;

    constexpr double SAMPLE_RATE = 44100;
    constexpr unsigned long FRAMES_PER_BUFFER = 32;

    #define AUDIO_DATA_TYPE int32_t

    boost::sync_bounded_queue<std::string> input_buffer{128};
    boost::sync_bounded_queue<std::string> output_buffer{128};

    
    unsigned long long input_dropped_frames = 0;
    unsigned long long output_dropped_frames = 0;

    int input_callback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
    {
        static AUDIO_DATA_TYPE input_tmp_buffer[FRAMES_PER_BUFFER+1];
        if(input_buffer.full()) // dropping frames
        {
            input_dropped_frames++;
            return 0;
        }
        for(unsigned int i = 0;i<frameCount;i++)
        {
            input_tmp_buffer[i] = ((AUDIO_DATA_TYPE*)input)[i];
        }
        input_tmp_buffer[frameCount] = 0;
        auto encoded = base64_encode(std::string_view((char*)input_tmp_buffer));
        input_buffer.push(encoded);
        return 0;
    }
    int output_callback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
    {//we assume that data is already binary
        std::string encoded;
        if(!output_buffer.try_pull(encoded))
            return 0;
        auto decoded = base64_decode(encoded);
        if(decoded.length() != frameCount*sizeof(AUDIO_DATA_TYPE))
            return 0;
        auto len = frameCount*sizeof(AUDIO_DATA_TYPE);
        for(unsigned int i = 0;i<len;i++)
        {
            ((char*)output)[i] = decoded[i];
        }
        return 0;
    }

    void comms_init()
    {
        Pa_Initialize();
        PaStreamParameters input_stream_params;
        input_stream_params.channelCount = 1;
        input_stream_params.device = Pa_GetDefaultInputDevice();
        input_stream_params.hostApiSpecificStreamInfo = nullptr;
        input_stream_params.sampleFormat = paInt32;
        //check for error from Pa_GetDefaultInputDevice
        auto input_device = Pa_GetDeviceInfo(input_stream_params.device);
        input_stream_params.suggestedLatency = input_device->defaultLowInputLatency;
        PaStreamParameters output_stream_params;
        output_stream_params.channelCount = 1;
        output_stream_params.device = Pa_GetDefaultOutputDevice();
        output_stream_params.hostApiSpecificStreamInfo = nullptr;
        output_stream_params.sampleFormat = paInt32;
        //check for error from Pa_GetDefaultOutputDevice
        auto output_device = Pa_GetDeviceInfo(output_stream_params.device);
        output_stream_params.suggestedLatency = output_device->defaultLowOutputLatency;
        
        Pa_OpenStream(&input_stream,&input_stream_params,nullptr,SAMPLE_RATE,FRAMES_PER_BUFFER,paNoFlag,input_callback,nullptr);
        Pa_OpenStream(&output_stream,nullptr,&output_stream_params,SAMPLE_RATE,FRAMES_PER_BUFFER,paNoFlag,output_callback,nullptr);

        Pa_StartStream(input_stream);
        Pa_StopStream(output_stream);

    }
    void comms_stop()
    {
        Pa_StopStream(input_stream);
        Pa_StopStream(output_stream);

        Pa_CloseStream(input_stream);
        Pa_CloseStream(output_stream);

        Pa_Terminate();
        audio_buddy.name = "";
        pending_name = "";
    }

    void audio()
    {
        while(true)
        {
            auto item = audio_queue.pull();
            auto args = parsing::msg_split(item.msg);
            std::unique_lock lock(name_mutex);
            if(args.size() == 1 && args[0] == "AUDIOSTART")
            {
                if(audio_buddy.name.length() == 0)
                {//no user connected for voice
                    network::udp::send(parsing::compose_message({"AUDIOACCEPT"}),item.src_endpoint);
                    audio_buddy = {item.src,item.src_endpoint};
                    comms_init();
                }else
                {//user already connected for voice
                    network::udp::send(parsing::compose_message({"AUDIOSTOP"}),item.src_endpoint);
                }
            }else if(args.size() == 1 && args[0] == "AUDIOACCEPT" && pending_name == item.src && audio_buddy.name.length()==0)
            {
                audio_buddy = {item.src,item.src_endpoint};
                pending_name = "";
                comms_init();
            }else if(args.size() == 1 && args[0] == "AUDIOSTOP" && (audio_buddy.name == item.src || pending_name == item.src))
            {
                comms_stop();
            }else if(args.size() == 2 && args[0] == "AUDIO" && audio_buddy.name == item.src)
            {
                if(!output_buffer.try_push(args[1]))
                    output_dropped_frames++;
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
        if(audio_buddy.name.length() == 0)
        {
            pending_name = name;
            network::udp::send(parsing::compose_message({"AUDIOSTART"}),name);
        }
    }
    void stop_call()
    {
        std::unique_lock lock(name_mutex);
        if(audio_buddy.name.length() != 0)
        {
            network::udp::send(parsing::compose_message({"AUDIOSTOP"}),audio_buddy.endpoint);\
            comms_stop();
        }
    }
}