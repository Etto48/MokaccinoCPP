#pragma once
#include <string>
#include <vector>
namespace network::audio
{
    extern unsigned long long input_dropped_frames;
    extern unsigned long long output_dropped_frames;

    /**
     * @brief initialize the module
     * 
     * @param whitelist list of users to automatically accept
     * @param default_action what to do if a user is not in the whitelist
     */
    void init(const std::vector<std::string>& whitelist, const std::string& default_action);
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