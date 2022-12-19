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
    /**
     * @brief strip ansi escape sequences (CSI) from a string
     * 
     * @param str string from where the codes will be stripped
     * @return stripped string
     */
    std::string strip_ansi(const std::string& str);
    /**
     * @brief get an environment variable and put it in a sring
     * 
     * @param env_var the name of the variable
     * @return the value of the variable, "" if not found
     */
    std::string getenv(const std::string& env_var);
    /**
     * @brief calculate the corresponding position in a string containing 
     * ansi escape sequences given the position in the same string withoout escape sequences
     * 
     * @param str ansi escaped string
     * @param len position in the non ansi equivalent (you can retrieve this with strip_ansi)
     * @return the position in the ansi escaped string
     */
    size_t ansi_len(const std::string& str, size_t len);
    /**
     * @brief remove the ansi escape codes from a string and set the flags for every char
     * to print it correctly with curses
     * 
     * @param str the input string with ansi escape codes
     * @return a vector of pairs (char, color info)
     */
    std::vector<std::pair<char,int>> curses_split_color(const std::string& str);
    /**
     * @brief sign the message m and append the message signature to it
     * 
     * @param m message to sign
     */
    void sign_and_append(std::string& m);
    /**
     * @brief check if the message sm ('<m> <signature>') is correctly signed
     * 
     * @param sm message with signature appended
     * @param name the name of the peer that sent the message
     * @return true if the signature is valid
     * @return false if the signature is not valid
     */
    bool verify_signature_from_message(const std::string& sm, const std::string& name);
    /**
     * @brief remove from a file name every part of the path and every char that can lead to a malfunction
     * 
     * @param fn original file name
     * @return clean file name
     */
    std::string clean_file_name(const std::string& fn);
}