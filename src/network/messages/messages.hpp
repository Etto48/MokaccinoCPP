#pragma once
#include <string>

namespace network::messages
{
    /**
     * @brief initialize the module
     * 
     */
    void init();
    /**
     * @brief send a text message to a logged user
     * 
     * @param name the destination
     * @param msg the text message
     */
    void send(const std::string& name, const std::string& msg);
}