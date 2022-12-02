#include "audio.hpp"
#include <portaudio.h>
namespace audio
{
    void audio()
    {
        
        auto err = Pa_Initialize();
        if(err!=paNoError)
        {
            logging::log("ERR",Pa_GetErrorText(err));
            return;
        }

        
        auto input_device = Pa_GetDefaultInputDevice();
        if(input_device == paNoDevice)
        {
            logging::log("ERR","No input device found");
            return;
        }
        auto output_device = Pa_GetDefaultOutputDevice();
        if(output_device == paNoDevice)
        {
            logging::log("ERR","No output device found");
            return;
        }

        auto input = Pa_GetDeviceInfo(input_device);
        auto output = Pa_GetDeviceInfo(output_device);

        logging::log("DBG",std::string("Audio input device: ") + input->name + ", latency: " + std::to_string(input->defaultLowInputLatency));
        logging::log("DBG",std::string("Audio output device: ") + output->name + ", latency: " + std::to_string(input->defaultLowOutputLatency));

        PaStream* input_stream;
        PaStream* output_stream;
        PaStreamParameters input_params;
        input_params.channelCount = 1;
        input_params.device = input_device;
        input_params.hostApiSpecificStreamInfo = nullptr;
        input_params.sampleFormat = paInt32;
        input_params.suggestedLatency = input->defaultLowInputLatency;
        PaStreamParameters output_params;
        output_params.channelCount = 1;
        output_params.device = output_device;
        output_params.hostApiSpecificStreamInfo = nullptr;
        output_params.sampleFormat = paInt32;
        output_params.suggestedLatency = output->defaultLowOutputLatency;

        constexpr unsigned long FRAMES_PER_BUFFER = 16;

        Pa_OpenStream(&input_stream,&input_params,nullptr,44100,FRAMES_PER_BUFFER,0,nullptr,nullptr);
        Pa_OpenStream(&output_stream,nullptr,&output_params,44100,FRAMES_PER_BUFFER,0,nullptr,nullptr);

        Pa_StartStream(input_stream);
        Pa_StartStream(output_stream);


        int32_t read_buffer[FRAMES_PER_BUFFER];
        while(true)
        {
            Pa_ReadStream(input_stream,read_buffer,FRAMES_PER_BUFFER);
            Pa_WriteStream(output_stream,read_buffer,FRAMES_PER_BUFFER);
        }
        
        Pa_StopStream(input_stream);
        Pa_StopStream(output_stream);

        Pa_CloseStream(input_stream);
        Pa_CloseStream(output_stream);
        Pa_Terminate();
    }
    void init()
    {
        //multithreading::add_service("audio",audio);
    }
}