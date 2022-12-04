#pragma once
#include <string>
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