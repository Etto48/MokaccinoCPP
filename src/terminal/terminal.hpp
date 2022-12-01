#pragma once
#include "../defines.hpp"

#include <iostream>
#include "../parsing/parsing.hpp"
#include "../network/connection/connection.hpp"
#include "../network/messages/messages.hpp"
#include "../multithreading/multithreading.hpp"
#include "prompt.hpp"
namespace terminal
{
    /**
     * @brief parse and process a terminal command
     * 
     * @param line the input line to parse
     * @return true if the command succeeded
     * @return false if the command failed
     */
    bool process_command(const std::string& line);
    /**
     * @brief initialize the module
     * 
     */
    void init();
};