#pragma once
#include <vector>
#include <string>

namespace terminal::commands
{
    bool connect(const std::vector<std::string>& args);
    bool disconnect(const std::vector<std::string>& args);
    bool msg(const std::vector<std::string>& args);
    bool voice(const std::vector<std::string>& args);
    bool sleep(const std::vector<std::string>& args);
    bool user(const std::vector<std::string>& args);
};