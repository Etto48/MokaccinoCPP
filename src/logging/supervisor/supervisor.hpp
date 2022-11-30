#pragma once
#include "../../defines.hpp"
#include <boost/thread.hpp>
#include "../logging.hpp"
#include "../../multithreading/multithreading.hpp"
#include "../../network/udp/udp.hpp"
namespace logging::supervisor
{
    void init(unsigned int timeout_seconds);
}