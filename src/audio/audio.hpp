#pragma once
#include <string>

namespace audio
{
    extern unsigned long long input_dropped_frames;
    extern unsigned long long output_dropped_frames;

    /**
     * @brief initialize the module
     * 
     */
    void init();
    /**
     * @brief start a voice call with a connected user if no other call is happening
     * 
     * @param name the user you want to call
     */
    void start_call(const std::string& name);
    /**
     * @brief stop a voice call if there was one
     * 
     */
    void stop_call();
}