#include "prompt.hpp"
#include "../defines.hpp"
#include "../ansi_escape.hpp"
#include "../ui/ui.hpp"
#include <iostream>
namespace terminal
{
    bool last_ret = true;
    void prompt()
    {
        #ifndef _TEST
        #ifndef NO_TERMINAL_UI
        ui::prompt("Command: ");
        #else
        #ifndef NO_ANSI_ESCAPE
        std::cout << "\r" CLEAR_LINE;
        if(last_ret)
            std::cout << CORRECT "O" RESET "> ";
        else
            std::cout << WRONG "X" RESET "> ";
        std::cout.flush();
        #endif
        #endif
        #endif
    }
}