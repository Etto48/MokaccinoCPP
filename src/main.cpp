#include <iostream>
#include "network/udp/udp.hpp"
#include "multithreading/multithreading.hpp"

int main()
{
    network::udp::init(23232);
    multithreading::join();
    return 0;
}