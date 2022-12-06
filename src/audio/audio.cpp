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

    PaStream* input_stream = nullptr;
    PaStream* output_stream = nullptr;

    constexpr double SAMPLE_RATE = 44100;
    constexpr unsigned long FRAMES_PER_BUFFER = 64;
    constexpr unsigned long ENCODED_FRAMES_SIZE = 344;

    #define AUDIO_DATA_TYPE int32_t
    struct DecodedBuffer
    {
        uint8_t data[FRAMES_PER_BUFFER*sizeof(AUDIO_DATA_TYPE)];
    };
    struct EncodedBuffer
    {
        unsigned char data[ENCODED_FRAMES_SIZE+1];
    };

    boost::sync_bounded_queue<std::string> input_buffer{128};
    boost::sync_bounded_queue<std::string> output_buffer{128};
    
    unsigned long long input_dropped_frames = 0;
    unsigned long long output_dropped_frames = 0;

    int input_callback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
    {
        static EncodedBuffer encoded;
        if(input_buffer.full()) // dropping frames
        {
            input_dropped_frames++;
            return 0;
        }
        b64_encode((unsigned char*)input,frameCount*sizeof(AUDIO_DATA_TYPE),encoded.data);
        input_buffer.push(std::string((char*)encoded.data));
        return 0;
    }
    int output_callback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
    {//we assume that data is already binary
        static std::string encoded;
        if(not output_buffer.try_pull(encoded))
            return 0;
        
        b64_decode((unsigned char*)encoded.c_str(),(unsigned int)encoded.length(),(unsigned char*)output);
        return 0;
    }

    class AudioError: public std::exception
    {
    public:
        std::string why;
        AudioError(const std::string& why):why(why){};
    };
    void comms_init()
    {
        try
        {
            if(Pa_Initialize()!=paNoError)
                throw AudioError("Pa_Initialize");
            PaStreamParameters input_stream_params;
            input_stream_params.channelCount = 1;
            input_stream_params.device = Pa_GetDefaultInputDevice();
            input_stream_params.hostApiSpecificStreamInfo = nullptr;
            input_stream_params.sampleFormat = paInt32;
            if(input_stream_params.device == paNoDevice)
                throw AudioError("Pa_GetDefaultInputDevice");
            auto input_device = Pa_GetDeviceInfo(input_stream_params.device);
            input_stream_params.suggestedLatency = input_device->defaultLowInputLatency;
            PaStreamParameters output_stream_params;
            output_stream_params.channelCount = 1;
            output_stream_params.device = Pa_GetDefaultOutputDevice();
            output_stream_params.hostApiSpecificStreamInfo = nullptr;
            output_stream_params.sampleFormat = paInt32;
            if(output_stream_params.device == paNoDevice)
                throw AudioError("Pa_GetDefaultOutputDevice");
            auto output_device = Pa_GetDeviceInfo(output_stream_params.device);
            output_stream_params.suggestedLatency = output_device->defaultLowOutputLatency;
            
            if(Pa_OpenStream(&input_stream,&input_stream_params,nullptr,SAMPLE_RATE,FRAMES_PER_BUFFER,paNoFlag,input_callback,nullptr) != paNoError)
                throw AudioError("Pa_OpenStream");
            if(Pa_OpenStream(&output_stream,nullptr,&output_stream_params,SAMPLE_RATE,FRAMES_PER_BUFFER,paNoFlag,output_callback,nullptr) != paNoError)
                throw AudioError("Pa_OpenStream");

            if(Pa_StartStream(input_stream) != paNoError)
                throw AudioError("Pa_StartStream");
            if(Pa_StartStream(output_stream) != paNoError)
                throw AudioError("Pa_StartStream");
        }catch(AudioError& e)
        {
            if(e.why != "Pa_Initialize") 
            {
                if(input_stream != nullptr)
                {
                    Pa_StopStream(input_stream);
                    Pa_CloseStream(input_stream);
                }
                if(output_stream != nullptr)
                {
                    Pa_StopStream(output_stream);
                    Pa_CloseStream(output_stream);
                }
                Pa_Terminate();
            }
            
            audio_buddy.name = "";
            pending_name = "";
            input_stream = nullptr;
            output_stream = nullptr;

            logging::audio_call_error_log(e.why);
        }
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
        input_stream = nullptr;
        output_stream = nullptr;
    }
    void audio_sender() {
        while(true)
        {
            auto encoded = input_buffer.pull();
            std::unique_lock lock(name_mutex);
            if(audio_buddy.name.length() > 0)
            {//we are connected
                network::udp::send(parsing::compose_message({"AUDIO",encoded}),audio_buddy.endpoint);
            }
        }
    }
    void audio()
    {
        while(true)
        {
            auto item = audio_queue.pull();
            auto args = parsing::msg_split(item.msg);
            std::unique_lock lock(name_mutex);
            if(args.size() == 1 and args[0] == "AUDIOSTART")
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
            }else if(args.size() == 1 and args[0] == "AUDIOACCEPT" and pending_name == item.src and audio_buddy.name.length()==0)
            {
                audio_buddy = {item.src,item.src_endpoint};
                pending_name = "";
                comms_init();
            }else if(args.size() == 1 and args[0] == "AUDIOSTOP" and (audio_buddy.name == item.src or pending_name == item.src))
            {
                comms_stop();
            }else if(args.size() == 2 and args[0] == "AUDIO" and audio_buddy.name == item.src)
            {
                if(not output_buffer.try_push(args[1]))
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
        multithreading::add_service("audio_sender",audio_sender);
    }
    void start_call(const std::string& name)
    {
        std::unique_lock lock(name_mutex);
        if(DEBUG and name == "loopback")
        {
            audio_buddy = {"loopback",network::udp::connection_map["loopback"].endpoint};
            comms_init();
        }
        else if(audio_buddy.name.length() == 0)
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