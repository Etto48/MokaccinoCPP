#include "prompt.hpp"
#include "../defines.hpp"
#include "../ansi_escape.hpp"
#include <iostream>
namespace terminal
{
    bool last_ret = true;
    void prompt()
    {
        #ifndef _TEST
        std::cout << "\r" CLEAR_LINE;
        if(last_ret)
            std::cout << CORRECT "O" RESET "> ";
        else
            std::cout << WRONG "X" RESET "> ";
        std::cout.flush();
        #endif
    }
}