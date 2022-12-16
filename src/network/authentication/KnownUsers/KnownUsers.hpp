#pragma once
#include <string>
#include <map>
#include <mutex>
#include <exception>
#include "../../../toml.hpp"
namespace network::authentication
{
    class KnownUsers
    {
    private:
        std::mutex users_mutex;
        toml::table users;
        std::string filename;
        void _save();
    public:
        KnownUsers() = default;
        void load(const std::string& filename);
        void save();
        bool add_key(const std::string& name, const std::string& key);
        bool replace_key(const std::string& name, const std::string& key);
        class KeyNotFound: public std::exception
        {
        public:
            const char* what(){return "KeyNotFound";}
        };
        std::string get_key(const std::string& name);
    };
}