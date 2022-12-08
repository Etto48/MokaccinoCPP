#include <toml.hpp>

toml::table test_config = toml::table{
    {"network", toml::table{
        { "username", "mokaccino"}
        }}
};

int test()
{
    return 0;
}