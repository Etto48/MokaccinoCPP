#pragma once
#include "../defines.hpp"

#include <iostream>
#include "../parsing/parsing.hpp"
#include "../network/connection/connection.hpp"
#include "../multithreading/multithreading.hpp"
namespace terminal
{
    bool process_command(const std::string& line);
    void init();
};