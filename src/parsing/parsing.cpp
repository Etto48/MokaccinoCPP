#include "parsing.hpp"
#ifndef MSG_SPLITTING_CHAR
    #define MSG_SPLITTING_CHAR ' '
#endif
#ifndef ESCAPE_CHAR
    #define ESCAPE_CHAR '\\'
#endif
namespace parsing
{
    std::string get_msg_keyword(const std::string& msg)
    {
        std::string ret;
        ret.reserve(msg.length());
        for(const auto& c : msg)
        {
            if(c == MSG_SPLITTING_CHAR)
                break;
            else
                ret+=c;
        }
            return ret;
    }
    std::vector<std::string> split(const std::string& str,char split_on=MSG_SPLITTING_CHAR, char escape_on=ESCAPE_CHAR)
    {
        std::vector<std::string> ret;
        std::string current;
        current.reserve(str.length());
        bool escape = false;
        for(const auto& c: str)
        {
            if(c == split_on && !escape)
            {
                if(current.length() > 0)
                {
                    ret.emplace_back(current);
                    current = "";
                }
            }
            else if(c == escape_on && !escape)
            {
                escape = true;
            }
            else
            {
                current += c;
                escape = false;
            }
        }
        if(current.length() > 0)
        {
            ret.emplace_back(current);
        }
        return ret;
    }
    std::vector<std::string> msg_split(const std::string& msg)
    {
        return split(msg,MSG_SPLITTING_CHAR,ESCAPE_CHAR);
    }
    std::vector<std::string> cmd_args(const std::string& line)
    {
        return split(line,' ','\\');
    }
    std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) 
    {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        return str;
    }
    const char* EndpointFromStrError::what()
    {
        return "Error converting the string to an endpoint";
    }
    boost::asio::ip::udp::endpoint endpoint_from_str(const std::string& str)
    {
        std::string addr, port;
        bool addr_done = false;
        for(const auto& c: str)
        {
            if(c!=':')
                addr_done = true;
            else if(!addr_done)
                addr += c;
            else
                port += c;
        }
        if(!addr_done)
            throw EndpointFromStrError{};
        boost::system::error_code ec;
        
        uint16_t port_var;
        auto addr_var = boost::asio::ip::address::from_string(addr,ec);
        if(ec)
        {
            throw EndpointFromStrError{};
        }
        try
        {
            port_var = std::stoi(port);   
        }catch(const std::exception&)
        {
            throw EndpointFromStrError{};
        }
        
        return boost::asio::ip::udp::endpoint{addr_var,port_var};
    }
    std::string compose_message(const std::vector<std::string>& components)
    {
        std::string ret;
        for(unsigned int i = 0; i<components.size(); i++)
        {
            auto new_comp = ReplaceAll(components[i],"\\","\\\\");
            new_comp = ReplaceAll(new_comp," ","\\ ");
            ret += new_comp;
            if(i!=components.size()-1)
            ret += MSG_SPLITTING_CHAR;
        }
        return ret;
    }
    std::pair<boost::asio::ip::udp::endpoint,std::string> endpoint_from_hostname(const std::string& str)
    {
        std::string host, port = DEFAULT_PORT_STR;
        uint16_t port_var = DEFAULT_PORT;
        if(str.find(':') != std::string::npos)
        {
            host = str.substr(0,str.find(':'));
            port = str.substr(str.find(':')+1,str.length()-host.length()-1);
        }
        else
        {
            host = str;
        }
        try{
            port_var = std::stoi(port);
        }catch(std::exception&)
        {
            port_var = DEFAULT_PORT;
        }

        auto endpoint = network::udp::dns_lookup(host,port_var);
        return {endpoint,host};
    }
}