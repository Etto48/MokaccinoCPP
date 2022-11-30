#pragma once
#include <boost/asio/ip/udp.hpp>
#include <format>
#define IP_VERSION boost::asio::ip::udp::v4()
#define DEFAULT_PORT 23232
#define DEFAULT_PORT_STR "23232"

#ifdef __APPLE__
    #define _format(...) "std::format NOT found"
#else
    #define _format(...) std::format(__VA_ARGS__)
#endif