#include "KnownUsers.hpp"
#include "../../../defines.hpp"
#include "../../../ansi_escape.hpp"
#include <fstream>
#include "../../../logging/logging.hpp"
namespace network::authentication
{
    void KnownUsers::load(const std::string& filename)
    {
        std::unique_lock lock(users_mutex);
        this->filename = filename;
        try
        {
            users = toml::parse_file(filename);   
            logging::log("DBG","Know users info loaded from \"" HIGHLIGHT +filename+ RESET "\"");
        }
        catch(const toml::parse_error&)
        {
            std::fstream file(filename,std::ios::out|std::ios::trunc);
            users = toml::table{};
            file << users;
            logging::log("DBG","Know users info created at \"" HIGHLIGHT +filename+ RESET "\"");
        }
    }
    void KnownUsers::_save()
    {
        std::fstream file(filename,std::ios::out|std::ios::trunc);
        file << users;
    }
    void KnownUsers::save()
    {
        std::unique_lock lock(users_mutex);
        _save();
    }
    bool KnownUsers::add_key(const std::string& name, const std::string& key)
    {
        std::unique_lock lock(users_mutex);
        if(users.find(name) != users.end())
            return false;
        users.emplace(name,toml::table{{"key",key}});
        _save();
        return true;
    }
    bool KnownUsers::replace_key(const std::string& name, const std::string& key)
    {
        std::unique_lock lock(users_mutex);
        auto ret = users.find(name) != users.end();
        users.emplace(name,toml::table{{"key",key}});
        _save();
        return ret;
    }
    bool KnownUsers::delete_key(const std::string& name)
    {
        std::unique_lock lock(users_mutex);
        if(users.find(name) == users.end())
            return false;
        users.erase(name);
        _save();
        return true;
    }
    std::vector<std::string> KnownUsers::get_all()
    {
        std::vector<std::string> ret;
        std::unique_lock lock(users_mutex);
        for(auto& [name, info]: users)
        {
            if(info.is_table())
                ret.emplace_back(name.str());
        }
        return ret;
    }
    std::string KnownUsers::get_key(const std::string& name)
    {
        std::unique_lock lock(users_mutex);
        auto ret = users[name]["key"].value_or<std::string>("");
        if(ret.length()==0 and users.find(name) == users.end())
            throw KeyNotFound{};
        return ret;
    }
}