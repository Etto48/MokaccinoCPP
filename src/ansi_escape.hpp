#pragma once

#ifndef NO_ANSI_ESCAPE
    #define ERR_TAG         "\033[31m"
    #define DBG_TAG         "\033[33m"
    #define MSG_TAG         "\033[34m"
    #define HIGHLIGHT       "\033[35m"
    #define TAG             "\033[36m"
    #define RESET           "\033[0m"
    #define CLEAR_LINE      "\033[K"
    #define MESSAGE_PEER    "\033[32m"
    #define MESSAGE_TEXT    "\033[90m"
    #define CORRECT         "\033[32m"
    #define WRONG           "\033[31m"
    #define CONTINUE_TAG    "\033[90m"
    #else
    #define ERR_TAG         ""
    #define DBG_TAG         ""
    #define MSG_TAG         ""
    #define HIGHLIGHT       ""
    #define TAG             ""
    #define CLEAR_LINE      ""
    #define RESET           ""
    #define MESSAGE_PEER    ""
    #define MESSAGE_TEXT    ""
    #define CORRECT         ""
    #define WRONG           ""
    #define CONTINUE_TAG    ""
    #endif