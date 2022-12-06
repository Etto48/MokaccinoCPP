#pragma once
#include <boost/asio/ip/udp.hpp>
#include "parsing/parsing.hpp"
#define IP_VERSION boost::asio::ip::udp::v4()
#define DEFAULT_PORT 23232
#define DEFAULT_PORT_STR "23232"
#ifdef _WIN32
#define MOKACCINO_ROOT (parsing::getenv("USERPROFILE")+"\\AppData\\Local\\Mokaccino")
#else
#define IsDebuggerPresent() false
#define MOKACCINO_ROOT (parsing::getenv("HOME")+"/.config/Mokaccino")
#endif
#define CONFIG_PATH (MOKACCINO_ROOT+"/mokaccino.cfg")

#define not !
#define or ||
#define and &&

extern bool DEBUG;