#pragma once
#include <boost/asio/ip/udp.hpp>
#include "parsing/parsing.hpp"
#define IP_VERSION boost::asio::ip::udp::v4()
#define DEFAULT_PORT 23232
#define DEFAULT_PORT_STR "23232"
#define NONCE_NON_ENCODED_LENGTH 36

#ifdef _WIN32
#define MOKACCINO_ROOT (parsing::getenv("USERPROFILE")+"\\AppData\\Local\\Mokaccino\\")
#define not !
#define or ||
#define and &&
#else
#define IsDebuggerPresent() false
#define MOKACCINO_ROOT (parsing::getenv("HOME")+"/.config/Mokaccino/")
#endif

#define CONFIG_PATH (MOKACCINO_ROOT+"mokaccino.cfg")
#define PRIVKEY_PATH (MOKACCINO_ROOT+"private.pem")
#define PUBKEY_PATH (MOKACCINO_ROOT+"public.pem")
#define DEFAULT_TIME_FORMAT (TAG "[" RESET "%H:%M:%S" TAG "]" RESET " ")
#define KNOWN_USERS_FILE (MOKACCINO_ROOT+"known_users.toml")

extern bool DEBUG;