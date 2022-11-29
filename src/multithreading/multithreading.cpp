#include "multithreading.hpp"
namespace multithreading
{
    std::map<std::string,boost::thread> services;
    void add_service(std::string name,void (*f)())
    {
        services[name] = boost::thread{f};
    }
    void join()
    {
        for(auto& th : services)
        {
            th.second.join();
        }
    }
}