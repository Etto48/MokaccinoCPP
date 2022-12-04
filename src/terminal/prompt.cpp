#include "prompt.hpp"
#include "../defines.hpp"
#include <iostream>
namespace terminal
{
    bool last_ret = true;
    void prompt()
    {
        std::cout << "\r\033[K";
        if(last_ret)
            std::cout << "\033[32mO\033[0m> ";
        else
            std::cout << "\033[31mX\033[0m> ";
        std::cout.flush();
    }
}