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
     * @param voice_volume_threshold the minimum volum to send when on a voice call
     */
    void init(const std::vector<std::string>& whitelist, const std::string& default_action, int16_t voice_volume_threshold=40);
    /**
     * @brief start a voice call with a connected user if no other call is happening
     * 
     * @param name the user you want to call
     * @return true if everything went right
     * @return false if user was not found or already in a call
     */
    bool start_call(const std::string& name);
    /**
     * @brief stop a voice call if there was one
     * 
     * @return true if you was connected to someone
     * @return false if you was not connected to anyone
     */
    bool stop_call();
}