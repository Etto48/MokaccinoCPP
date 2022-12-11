#pragma once
#include <string>
namespace ui
{
    void scroll_text(int line_count);
    std::string getline();
    void prompt(const std::string& str);
    void print(const std::string& str);
    void init();
    void fini();
}