#pragma once
#include <string>
#include <vector>
#include <tuple>
#include <boost/asio/ip/udp.hpp>

namespace parsing
{
    /**
     * @brief returns the keyword from a message string received from udp socket
     * 
     * @param msg the content of the string
     * @return the keyword
     */
    std::string get_msg_keyword(const std::string& msg);
    /**
     * @brief split a message in the various tokens it's made of, the split
     * is made on 
     * 
     * @param msg message to split
     * @return vector of every token included the keyword
     */
    std::vector<std::string> msg_split(const std::string& msg);
    /**
     * @brief split a command in the various tokens it's made of 
     * the inner workings are the same of msg_split
     * 
     * @param line command line to split
     * @return vector of every token
     */
    std::vector<std::string> cmd_args(const std::string& line);
    /**
     * @brief this exception is thrown by endpoint_from_str when it fail converting
     * the input string
     * 
     */
    class EndpointFromStrError: public std::exception
    {
    public:
        const char* what();
    };
    /**
     * @brief convert a string in the format 000.000.000.000:00000 in an endpoint
     * if the string cannot be converted it throws EndpointFromStrError
     * 
     * @param str the string to convert
     * @return the converted endpoint
     */
    boost::asio::ip::udp::endpoint endpoint_from_str(const std::string& str);
    /**
     * @brief escapes and concatenates the strings given as input, for example if the input is
     * {"MSG","test test test"} the output will be "MSG test\ test\ test"
     * this function is the reverse of msg_split
     * 
     * @param components the tokens to compose
     * @return the string made of the tokens and escaped
     */
    std::string compose_message(const std::vector<std::string>& components);
    /**
     * @brief convert a string like hostname[:port] into a boost endpoint
     * doing a DNS lookup, if it fails to resolve throws udp::LookupError
     * 
     * @param str string to convert
     * @return (enpoint, hostname)
     */
    std::pair<boost::asio::ip::udp::endpoint,std::string> endpoint_from_hostname(const std::string& str);
}