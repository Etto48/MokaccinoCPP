#include "defines.hpp"
#include <iostream>
#include <boost/program_options.hpp>
#include "network/udp/udp.hpp"
#include "multithreading/multithreading.hpp"
#include "logging/supervisor/supervisor.hpp"
#include "terminal/terminal.hpp"
#include "network/connection/connection.hpp"

int main(int argc, char* argv[])
{
    //PROGRAM OPTIONS
    boost::program_options::options_description desc {
        "Mokaccino is a simple P2P application that supports VoIP and text messages\n"
        "Allowed options"
    };
    boost::program_options::variables_map args;
    desc.add_options()
        ("help,h","print help message")
        ("port,p",boost::program_options::value<uint16_t>()->default_value(DEFAULT_PORT)->value_name("port"),
            "set a custom port for network communications");
    boost::program_options::store(boost::program_options::parse_command_line(argc,argv,desc),args);
    boost::program_options::notify(args);
    if(args.count("help"))
    {
        std::cout << desc << std::endl;
    }
    else
    {
        //INITIALIZATIONS
        logging::supervisor::init(60);
        network::udp::init(args["port"].as<uint16_t>());
        network::connection::init("ettore");
        terminal::init();
        
        //tests
        #ifdef _DEBUG
        network::udp::send("TEST","loopback");
        terminal::process_command("connect hostname localhost");
        #endif

        //WAIT FOR EXIT
        multithreading::wait_termination();
        multithreading::stop_all();
    }
    return 0;
}