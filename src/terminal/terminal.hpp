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
    bool process_command(const std::string& line);
    void init();
};