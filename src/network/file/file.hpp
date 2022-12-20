#pragma once
#include <exception>
#include <string>
namespace network::file
{
    /**
     * @brief get how many files are being transferred
     * 
     * @return file transfers count
     */
    size_t ongoing_transfers_count();
    class FileNotFound: public std::exception
    {
    public:
        const char* what(){return "FileNotFound";}
    };
    class UserNotFound: public std::exception
    {
    public:
        const char* what(){return "UserNotFound";}
    };
    /**
     * @brief start to send the file located at "path" to "to", throws FileNotFound if the file could not be opened
     * and UserNotFound if the user could not be found
     * 
     * @param to destination username
     * @param path path of the file to send
     * @return true if everything went ok
     * @return false if a file with the same hash is already sending
     */
    bool init_file_upload(const std::string& to, const std::string& path);
    /**
     * @brief initialize the module
     * 
     */
    void init();
}