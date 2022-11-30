#pragma once
#include <string>
#include "../../multithreading/multithreading.hpp"
#include "../../parsing/parsing.hpp"
#include "../udp/udp.hpp"
namespace network::messages
{
    void init();
    void send(const std::string& name, const std::string& msg);
}