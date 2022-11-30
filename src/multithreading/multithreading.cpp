#include "multithreading.hpp"
namespace multithreading
{
    std::map<std::string,boost::thread> services;
    std::mutex services_mutex;
    void add_service(std::string name,void (*f)())
    {
        std::unique_lock lock(services_mutex);
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
    size_t get_count()
    {
        std::unique_lock lock(services_mutex);
        return services.size();
    }
    std::mutex termination_mutex;
    std::condition_variable termination_required;
    bool termination_required_flag = false;
    void request_termination()
    {
        std::unique_lock lock(termination_mutex);
        termination_required_flag = true;
        termination_required.notify_all();
    }
    void wait_termination()
    {
        std::unique_lock lock(termination_mutex);
        while(!termination_required_flag)
            termination_required.wait(lock);
    }
}