#include "curses_ansi_lookup.hpp"
#include "../defines.hpp"
#include "../ansi_escape.hpp"
namespace parsing
{
    std::map<std::string,int> curses_lookup = {
            {RESET,         0},    
            {PROMPT_COLOR,  1},
            {HIGHLIGHT,     2},
            {ERR_TAG,       3},
            {MESSAGE_PEER,  4},
            {DBG_TAG,       5},
            {MSG_TAG,       6},
            {TAG,           7},
            {MESSAGE_TEXT,  8}
    };
}