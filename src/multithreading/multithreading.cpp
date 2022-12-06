#include "multithreading.hpp"
#include "../defines.hpp"
#include <map>
#include <mutex>
#include <condition_variable>
#include <boost/thread/thread.hpp>
#include <boost/asio/signal_set.hpp>
#include "../logging/logging.hpp"

namespace multithreading
{
    std::map<std::string,boost::thread> services;
    std::map<boost::thread::id,std::string> services_id_name_lookup;
    std::mutex services_mutex;
    void add_service(std::string name,void (*f)())
    {
        {
            std::unique_lock lock(services_mutex);
            services[name] = boost::thread{f};
            services_id_name_lookup[services[name].get_id()] = name;
        }
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
        while(not termination_required_flag)
            termination_required.wait(lock);
    }
    std::string get_current_thread_name()
    {
        std::unique_lock lock(services_mutex);
        
        auto id = boost::this_thread::get_id();
        auto ret = services_id_name_lookup.find(id);
        if(ret != services_id_name_lookup.end())
            return ret->second;
        else
            return "main";
    }
}