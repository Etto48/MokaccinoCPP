#include <toml.hpp>

toml::table test_config = toml::table{
    {"network", toml::table{
        { "username", "mokaccino"},
        { "connection", toml::table{
            {"whitelist", toml::array{ "user1", "user2", "user3" }},
            {"default_action", "refuse"}
        }},
        { "audio", toml::table{
            {"whitelist", toml::array{ "user1", "user2" }},
            {"default_action", "accept"}
        }}}
    },
    {"terminal", toml::table{
        {"startup_commands", toml::array{ "user list", "clearlywrongcommand", "msg loopback test test tesssst   test"}}}
    }
};

extern bool config_loaded;
int test()
{
    if(config_loaded)
        return 0;
    else
        return -1;
}