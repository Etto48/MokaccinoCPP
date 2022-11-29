#include <string>
#include <map>
#include <boost/thread/thread.hpp>

namespace multithreading
{
    void add_service(std::string name,void (*f)());
    void join();
}