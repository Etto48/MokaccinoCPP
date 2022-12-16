#include "parsing.hpp"
#include "../defines.hpp"
#include "../network/udp/udp.hpp"
#include "curses_ansi_lookup.hpp"
#include "../network/authentication/authentication.hpp"
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
            if(c == split_on and not escape)
            {
                if(current.length() > 0)
                {
                    ret.emplace_back(current);
                    current = "";
                }
            }
            else if(c == escape_on and not escape)
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
            else if(not addr_done)
                addr += c;
            else
                port += c;
        }
        if(not addr_done)
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
    std::string strip_ansi(const std::string& str)
    {
        std::string ret;
        bool stripping = false;
        for(const auto& c: str)
        {
            if(c=='\033')
            {
                stripping = true;
            }
            else if(stripping and std::isalpha(c))
            {
                stripping = false;
            }
            else if(not stripping)
            {
                ret += c;
            }
        }
        return ret;
    }
    std::string getenv(const std::string& env_var)
    {
        #ifdef _WIN32
        char *var = nullptr;
        std::string ret;
        if(_dupenv_s(&var,nullptr,env_var.c_str())!=0)
            return "";
        if(var != nullptr)
        {
            ret = var;
            free(var);
            return ret;
        }
        else
        {
            return "";
        }
        
        #else
        auto var = std::getenv(env_var.c_str());
        if(var == nullptr)
            return "";
        else return std::string(var);        
        #endif
    }
    size_t ansi_len(const std::string& str, size_t len)
    {
        size_t current = 0;
        size_t actual = 0;
        bool stripping = false;
        for(const auto& c: str)
        {
            if(current == len)
                break;
            if(c=='\033')
            {
                stripping = true;
            }
            else if(stripping and std::isalpha(c))
            {
                stripping = false;
            }
            else if(not stripping)
            {
                current += 1;
            }
            actual += 1;
        }
        return actual;
    }
    std::vector<std::pair<char,int>> curses_split_color(const std::string& str)
    {
        std::vector<std::pair<char,int>> ret;
        bool stripping = false;
        int current_tag = 0;
        std::string last_ansi_code = "";
        for(const auto& c: str)
        {
            if(c=='\033')
            {
                stripping = true;
                last_ansi_code += c;
            }
            else if(stripping and std::isalpha(c))
            {
                stripping = false;
                last_ansi_code += c;
                auto curses_code = curses_lookup.find(last_ansi_code);
                if(curses_code != curses_lookup.end())
                {
                    current_tag = curses_code->second;
                }
                last_ansi_code = "";
            }
            else if(not stripping)
            {
                ret.emplace_back(std::pair{c,current_tag});
            }
            else
            {
                last_ansi_code += c;
            }
        }
        return ret;
    }
    void sign_and_append(std::string& m)
    {
        auto signature = network::authentication::sign(m);
        m = m + MSG_SPLITTING_CHAR + signature;
    }
    bool verify_signature_from_message(const std::string& sm, const std::string& name)
    {
        auto last_space = sm.find_last_of(MSG_SPLITTING_CHAR);
        if(last_space == sm.npos)
            return false;
        auto m = sm.substr(0,last_space);
        auto signature = sm.substr(last_space+1);
        while (m.ends_with(MSG_SPLITTING_CHAR))
        {
            if(m.length()<2)
                return false;
            if(m[m.length()-2] != ESCAPE_CHAR)
                m.pop_back();
        }
        return network::authentication::verify(m,signature,name);   
    }
}