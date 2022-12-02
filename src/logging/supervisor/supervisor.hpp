#pragma once
#include "../../defines.hpp"
#include <boost/thread.hpp>
#include "../logging.hpp"
#include "../../multithreading/multithreading.hpp"
#include "../../network/udp/udp.hpp"
namespace logging::supervisor
{
    /**
     * @brief initialize the module
     * this module is disabled if DEBUG is not set
     * 
     * @param timeout_seconds time to wait between one one log and another
     */
    void init(unsigned int timeout_seconds);
}