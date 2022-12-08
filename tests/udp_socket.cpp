#include <defines.hpp>
#include <network/udp/udp.hpp>
#include <string>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_service.hpp>
#include <logging/logging.hpp>
#include <toml.hpp>

toml::table test_config = toml::table{
    {"network", toml::table{
        { "username", "mokaccino"}
        }}
};

int test()
{
    boost::asio::io_service io_service;
    boost::asio::ip::udp::socket test_socket{io_service};
    auto localhost = boost::asio::ip::address::from_string("127.0.0.1");
    boost::asio::ip::udp::endpoint recv_endpoint(localhost,23233);
    test_socket.open(boost::asio::ip::udp::v4());
    test_socket.bind(recv_endpoint);

    std::string send_data = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    network::udp::send(send_data,recv_endpoint);

    auto recv_data = new char[send_data.length()+2];
    boost::asio::ip::udp::endpoint sender;
    test_socket.receive_from(boost::asio::buffer(recv_data,send_data.length()+1),sender);
    recv_data[send_data.length()+1] = '\0';
    bool success = false;
    if(send_data+"\n"==std::string(recv_data))
        success = true;
    if(not success)logging::log("ERR","Received " + std::string(recv_data));
    delete[] recv_data;
    if(success) return 0;
    return -1;
}