#include "multithreading.hpp"
namespace multithreading
{
    std::map<std::string,boost::thread> services;
    void add_service(std::string name,void (*f)())
    {
        services[name] = boost::thread{f};
        logging::new_thread_log(name);
    }
    void join()
    {
        for(auto& th : services)
        {
            th.second.join();
        }
    }
    void stop(std::string name)
    {
        services[name].interrupt();
    }
    void stop_all()
    {
        for(auto& th : services)
        {
            th.second.interrupt();
        }
    }
}