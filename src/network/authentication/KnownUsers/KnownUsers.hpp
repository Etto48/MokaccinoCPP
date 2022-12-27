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
        /**
         * @brief load the object from a toml file
         * 
         * @param filename path to the file
         */
        void load(const std::string& filename);
        /**
         * @brief save the object to the previously opened file
         * 
         */
        void save();
        /**
         * @brief add a key if not already present and save to file
         * 
         * @param name the name of the user
         * @param key the key in the PEM format without the first and last line,
         * the key can also be "" to blacklist the user
         * @return true if the user was not present
         * @return false if the user was already present
         */
        bool add_key(const std::string& name, const std::string& key);
        /**
         * @brief add or replace a key even if there is already one associated
         * with the selected user and save to file
         * 
         * @param name the name of the user
         * @param key the key in PEM format without the first and last line, 
         * the key can also be "" to blacklist the user
         * @return true if the user was not present
         * @return false if the user was already present
         */
        bool replace_key(const std::string& name, const std::string& key);
        /**
         * @brief delete a key associated with a user
         * 
         * @param name the user name to remove from the list
         * @return true if the user was present
         * @return false if the user was not found
         */
        bool delete_key(const std::string& name);
        /**
         * @brief thrown by get_key when the user was not found
         * 
         */
        class KeyNotFound: public std::exception
        {
        public:
            const char* what(){return "KeyNotFound";}
        };
        /**
         * @brief get every user present in the object
         * 
         * @return a vector of every username
         */
        std::vector<std::string> get_all();
        /**
         * @brief get the key associated with a user, throws KeyNotFound if the user was not present
         * 
         * @param name the name of the user to search
         * @return the key in the PEM format without the first and last line, 
         * the key can also be "" if the user was blacklisted
         */
        std::string get_key(const std::string& name);
    };
}