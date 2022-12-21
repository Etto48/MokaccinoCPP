#pragma once
#include <vector>
#include <string>

namespace terminal::commands
{
    bool connect(const std::string& line, const std::vector<std::string>& args);
    bool disconnect(const std::string& line, const std::vector<std::string>& args);
    bool msg(const std::string& line, const std::vector<std::string>& args);
    bool voice(const std::string& line, const std::vector<std::string>& args);
    bool sleep(const std::string& line, const std::vector<std::string>& args);
    bool user(const std::string& line, const std::vector<std::string>& args);
    bool scroll(const std::string& line, const std::vector<std::string>& args);
    bool file(const std::string& line, const std::vector<std::string>& args);
};