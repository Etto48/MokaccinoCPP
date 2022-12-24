#include "audio.hpp"
#include "../../defines.hpp"
#include "../../ansi_escape.hpp"
#include <mutex>
#include <string_view>
#include <boost/thread/sync_bounded_queue.hpp>
#include <portaudio.h>
#include <opus/opus.h>
#include "../../terminal/terminal.hpp"
#include "../../logging/logging.hpp"
#include "../../multithreading/multithreading.hpp"
#include "../ConnectionAction/ConnectionAction.hpp"
#include "../MessageQueue/MessageQueue.hpp"
#include "../udp/udp.hpp"
#include "../../parsing/parsing.hpp"
#include "../../base64/base64.h"
namespace network::audio
{
    std::vector<std::string> whitelist;
    network::MessageQueue audio_queue;
    std::mutex name_mutex;
    std::string pending_name;
    struct AudioBuddy
    {
        std::string name = "";
        boost::asio::ip::udp::endpoint endpoint;
    };
    AudioBuddy audio_buddy;

    ConnectionAction default_action = ConnectionAction::PROMPT;

    PaStream* input_stream = nullptr;
    PaStream* output_stream = nullptr;

    OpusEncoder* encoder = nullptr;
    OpusDecoder* decoder = nullptr;

    unsigned char* input_encoder_buffer = nullptr;
    unsigned char* output_decoder_buffer = nullptr;

    uint16_t voice_threshold = 40;

    constexpr opus_int32 SAMPLE_RATE = 48000;
    
    constexpr unsigned long FRAMES_PER_BUFFER = 120;
    
    #define AUDIO_DATA_TYPE int16_t
    #define AUDIO_DATA_TYPE_PA paInt16

    constexpr opus_int32 BUFFER_OPUS_SIZE = FRAMES_PER_BUFFER*sizeof(AUDIO_DATA_TYPE)/4;
    unsigned long ENCODED_FRAMES_SIZE = b64e_size(BUFFER_OPUS_SIZE);

    struct DecodedBuffer
    {
        uint8_t data[FRAMES_PER_BUFFER*sizeof(AUDIO_DATA_TYPE)];
    };
    
    unsigned char* b64_encoded_buffer_data = nullptr;

    boost::sync_bounded_queue<std::string> input_buffer{256};
    boost::sync_bounded_queue<std::string> output_buffer{256};
    
    unsigned long long input_dropped_frames = 0;
    unsigned long long output_dropped_frames = 0;

    int16_t volume(const int16_t* input, unsigned long frameCount)
    {
        size_t ret = 0;
        for(auto v = input; v < input+frameCount; v++)
        {
            ret += std::abs(*v);
        }
        ret /= frameCount;
        return (int16_t)ret;
    }

    int input_callback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
    {
        if(input_buffer.full()) // dropping frames
        {
            input_dropped_frames++;
            return 0;
        }
        if(volume((const int16_t*)input,frameCount) < voice_threshold)
            return 0;
        auto encoded_size = opus_encode(encoder,(opus_int16*)input,frameCount,input_encoder_buffer,BUFFER_OPUS_SIZE);
        if(encoded_size < 0)
            return 0;
        b64_encode(input_encoder_buffer,encoded_size,b64_encoded_buffer_data);
        input_buffer.push(std::string((char*)b64_encoded_buffer_data));
        return 0;
    }
    int output_callback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
    {//we assume that data is already binary
        static std::string encoded;
        if(not output_buffer.try_pull(encoded))
        {
            memset(output,0,frameCount*sizeof(AUDIO_DATA_TYPE));
            return 0;
        }
        auto decoded_size = b64d_size((unsigned int)encoded.length());
        b64_decode((unsigned char*)encoded.c_str(),(unsigned int)encoded.length(),output_decoder_buffer);
        auto opus_decoded_size = opus_decode(decoder,output_decoder_buffer,decoded_size,(opus_int16*)output,frameCount,0);
        if(opus_decoded_size < 0)
            return 0;
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
            input_stream_params.sampleFormat = AUDIO_DATA_TYPE_PA;
            if(input_stream_params.device == paNoDevice)
                throw AudioError("Pa_GetDefaultInputDevice");
            auto input_device = Pa_GetDeviceInfo(input_stream_params.device);
            input_stream_params.suggestedLatency = input_device->defaultLowInputLatency;
            PaStreamParameters output_stream_params;
            output_stream_params.channelCount = 1;
            output_stream_params.device = Pa_GetDefaultOutputDevice();
            output_stream_params.hostApiSpecificStreamInfo = nullptr;
            output_stream_params.sampleFormat = AUDIO_DATA_TYPE_PA;
            if(output_stream_params.device == paNoDevice)
                throw AudioError("Pa_GetDefaultOutputDevice");
            auto output_device = Pa_GetDeviceInfo(output_stream_params.device);
            output_stream_params.suggestedLatency = output_device->defaultLowOutputLatency;
            
            input_encoder_buffer = new unsigned char[BUFFER_OPUS_SIZE];
            output_decoder_buffer = new unsigned char[BUFFER_OPUS_SIZE];
            b64_encoded_buffer_data = new unsigned char[ENCODED_FRAMES_SIZE + 1];

            int error = 0;
            encoder = opus_encoder_create(SAMPLE_RATE,1,OPUS_APPLICATION_VOIP,&error);
            if(error != OPUS_OK)
                throw AudioError("opus_encoder_create");
            decoder = opus_decoder_create(SAMPLE_RATE,1,&error);
            if(error != OPUS_OK)
                throw AudioError("opus_decoder_create");

            opus_encoder_ctl(encoder,OPUS_SET_BITRATE(OPUS_BITRATE_MAX));
            opus_decoder_ctl(decoder,OPUS_SET_BITRATE(OPUS_BITRATE_MAX));

            if(Pa_OpenStream(&input_stream,&input_stream_params,nullptr,double(SAMPLE_RATE),FRAMES_PER_BUFFER,paNoFlag,input_callback,nullptr) != paNoError)
                throw AudioError("Pa_OpenStream");
            if(Pa_OpenStream(&output_stream,nullptr,&output_stream_params,double(SAMPLE_RATE),FRAMES_PER_BUFFER,paNoFlag,output_callback,nullptr) != paNoError)
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

            if(input_encoder_buffer != nullptr)
            {
                delete [] input_encoder_buffer;
                input_encoder_buffer = nullptr;
            }
            if(output_decoder_buffer != nullptr)
            {
                delete [] output_decoder_buffer;
                output_decoder_buffer = nullptr;
            }
            if(b64_encoded_buffer_data != nullptr)
            {
                delete [] b64_encoded_buffer_data;
                b64_encoded_buffer_data = nullptr;
            }

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
        
        opus_encoder_destroy(encoder);
        opus_decoder_destroy(decoder);

        audio_buddy.name = "";
        pending_name = "";
        input_stream = nullptr;
        output_stream = nullptr;

        if(input_encoder_buffer != nullptr)
        {
            delete [] input_encoder_buffer;
            input_encoder_buffer = nullptr;
        }
        if(output_decoder_buffer != nullptr)
        {
            delete [] output_decoder_buffer;
            output_decoder_buffer = nullptr;
        }
        if(b64_encoded_buffer_data != nullptr)
        {
            delete [] b64_encoded_buffer_data;
            b64_encoded_buffer_data = nullptr;
        }
    }
    void _stop_call()
    {
        if(audio_buddy.name.length() != 0)
        {
            network::udp::send(parsing::compose_message({"AUDIOSTOP"}),audio_buddy.endpoint);\
            logging::log("MSG","Voice call with " HIGHLIGHT + audio_buddy.name + RESET " ended");
            comms_stop();
        }
    }
    void audio_sender() {
        while(true)
        {
            auto encoded = input_buffer.pull();
            std::unique_lock lock(name_mutex);
            if(network::udp::connection_map.check_user(audio_buddy.name))
            {
                if(audio_buddy.name.length() > 0)
                {//we are connected
                    network::udp::send(parsing::compose_message({"AUDIO",encoded}),audio_buddy.endpoint);
                }
            }else
            {// the other user was disconnected somehow
                _stop_call();
            }
        }
    }
    bool check_whitelist(const std::string& name)
    {
        for(auto& a: whitelist)
        {
            if(a==name)
                return true;
        }
        return false;
    }
    void accept_connection(const boost::asio::ip::udp::endpoint& endpoint, const std::string& name)
    {
        network::udp::send(parsing::compose_message({"AUDIOACCEPT"}),endpoint);
        audio_buddy = {name,endpoint};
        logging::log("MSG","Voice call accepted from " HIGHLIGHT + name + RESET);
        comms_init();
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
                    if(check_whitelist(item.src) or default_action == ConnectionAction::ACCEPT)
                    {
                        accept_connection(item.src_endpoint,item.src);
                    }else if(default_action == ConnectionAction::REFUSE)
                    {
                        logging::log("MSG","Voice call refused automatically from \"" HIGHLIGHT +item.src+ RESET "\"");
                        network::udp::send(parsing::compose_message({"AUDIOSTOP"}),item.src_endpoint);
                    }else
                    {
                        terminal::input("User \"" HIGHLIGHT+item.src+RESET "\" requested to start a voice call, accept? (y/n)",
                        [item](const std::string& input){
                            if(input == "Y" or input == "y")
                            {
                                accept_connection(item.src_endpoint,item.src);
                            }else
                            {
                                logging::log("MSG","Voice call refused from \"" HIGHLIGHT +item.src+ RESET "\"");
                                network::udp::send(parsing::compose_message({"AUDIOSTOP"}),item.src_endpoint);
                            }
                        });
                    }
                }else
                {//user already connected for voice
                    network::udp::send(parsing::compose_message({"AUDIOSTOP"}),item.src_endpoint);
                }
                logging::log("DBG","Handled " HIGHLIGHT + args[0] + RESET " from " HIGHLIGHT + item.src + RESET);
            }else if(args.size() == 1 and args[0] == "AUDIOACCEPT" and pending_name == item.src and audio_buddy.name.length()==0)
            {
                audio_buddy = {item.src,item.src_endpoint};
                pending_name = "";
                comms_init();
                logging::log("MSG","Voice call accepted from " HIGHLIGHT + item.src + RESET);
            }else if(args.size() == 1 and args[0] == "AUDIOSTOP" and (audio_buddy.name == item.src or pending_name == item.src))
            {
                if(audio_buddy.name.length()!=0)
                    logging::log("MSG","Voice call with " HIGHLIGHT +audio_buddy.name+ RESET " was stopped from the other peer");
                else
                    logging::log("MSG","Voice call refused from " HIGHLIGHT +pending_name+ RESET);
                comms_stop();
                logging::log("DBG","Handled " HIGHLIGHT + args[0] + RESET " from " HIGHLIGHT + item.src + RESET);
            }else if(args.size() == 2 and args[0] == "AUDIO" and audio_buddy.name == item.src)
            {
                if(not output_buffer.try_push(args[1]))
                    output_dropped_frames++;
            }
        }
    }
    void init(const std::vector<std::string>& whitelist, const std::string& default_action, int16_t voice_volume_threshold)
    {
        voice_threshold = voice_volume_threshold;
        audio::whitelist = whitelist;
        if(default_action == "accept")
        {
            audio::default_action = ConnectionAction::ACCEPT;
        }else if(default_action == "refuse")
        {
            audio::default_action = ConnectionAction::REFUSE;
        }else if(default_action == "prompt")
        {
            audio::default_action = ConnectionAction::PROMPT;
        }
        else
        {
            audio::default_action = ConnectionAction::PROMPT;
            logging::log("ERR","Error in configuration file at network.connection.default_action: this must be one of \"accept\", \"refuse\" or \"prompt\", \"prompt\" was selected as default");
        }

        network::udp::register_queue("AUDIOSTART",audio_queue,true);
        network::udp::register_queue("AUDIOACCEPT",audio_queue,true);
        network::udp::register_queue("AUDIOSTOP",audio_queue,true);
        network::udp::register_queue("AUDIO",audio_queue,true);

        multithreading::add_service("audio",audio);
        multithreading::add_service("audio_sender",audio_sender);
    }
    bool start_call(const std::string& name)
    {
        std::unique_lock lock(name_mutex);
        if(audio_buddy.name.length() == 0)
        {
            if(DEBUG and name == "loopback")
            {   try
                {
                    audio_buddy = {"loopback",udp::connection_map["loopback"].endpoint};
                    logging::log("MSG","Voice call accepted from " HIGHLIGHT "loopback" RESET);
                    comms_init();
                    return true;
                }catch(network::DataMap::NotFound&)
                {
                    logging::log("ERR","User " HIGHLIGHT "loopback" RESET " was not found");
                    return false;
                }
            }
            else
            {
                try
                {
                    auto endpoint = udp::connection_map[name].endpoint;
                    pending_name = name;
                    network::udp::send(parsing::compose_message({"AUDIOSTART"}),endpoint);
                    return true;
                }catch(network::DataMap::NotFound&)
                {
                    logging::user_not_found_log(name);
                    return false;
                }
            }
        }else
        {
            logging::log("ERR","You are already in a voice call with " HIGHLIGHT +audio_buddy.name+ RESET);
            return false;
        }
    }

    bool stop_call()
    {
        std::unique_lock lock(name_mutex);
        if(audio_buddy.name.length() != 0)
        {
            _stop_call();
            return true;
        }
        else
        {
            logging::log("ERR","You are not in a voice call");
            return false;
        }
    }
}