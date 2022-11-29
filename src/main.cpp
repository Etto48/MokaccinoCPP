#include <iostream>
#include "network/udp/udp.hpp"
#include "multithreading/multithreading.hpp"

boost::asio::io_service io_service{};
int main()
{
    network::udp::init(23232);

    //tests
    network::udp::send("PING","loopback");
    
    multithreading::join();
    return 0;
}