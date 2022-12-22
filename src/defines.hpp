#pragma once
#include <boost/asio/ip/udp.hpp>
#include "parsing/parsing.hpp"
#define IP_VERSION boost::asio::ip::udp::v4()
#define DEFAULT_PORT 23232
#define DEFAULT_PORT_STR "23232"
#define NONCE_NON_ENCODED_LENGTH 36
#define MAX_HISTORY_LINES 100

#ifdef _WIN32
#define HOME_PATH parsing::getenv("USERPROFILE")
#define MOKACCINO_ROOT (HOME_PATH+"\\AppData\\Local\\Mokaccino\\")
#define DOWNLOAD_PATH (HOME_PATH+"\\Downloads\\")
#define not !
#define or ||
#define and &&
#else
#define IsDebuggerPresent() false
#define HOME_PATH parsing::getenv("HOME")
#define MOKACCINO_ROOT (HOME_PATH+"/.config/Mokaccino/")
#define DOWNLOAD_PATH (HOME_PATH+"/Downloads/")
#endif

#define CONFIG_PATH (MOKACCINO_ROOT+"mokaccino.cfg")
#define PRIVKEY_PATH (MOKACCINO_ROOT+"private.pem")
#define PUBKEY_PATH (MOKACCINO_ROOT+"public.pem")
#define DEFAULT_TIME_FORMAT (TAG "[" RESET "%H:%M:%S" TAG "]" RESET " ")
#define KNOWN_USERS_FILE (MOKACCINO_ROOT+"known_users.toml")

extern bool DEBUG;