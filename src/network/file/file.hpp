#pragma once
#include <exception>
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <boost/date_time/posix_time/posix_time.hpp>
namespace network::file
{
    enum class FileTransferDirection
    {
        upload,
        download
    };
    struct FileTransferInfo
    {
        std::string file_name;
        std::string username;
        bool accepted;
        std::vector<unsigned char> data;
        std::vector<bool> received_chunks;
        // first byte not received (only for download) or first byte yet to send (only for upload)
        size_t next_sequence_number = 0;
        // last ack sent/received (up/down)
        size_t last_acked_number = 0;
        FileTransferDirection direction;
        boost::posix_time::ptime last_ack;
        static size_t chunk_count(size_t data_size);
        static FileTransferInfo prepare_for_upload(
            const std::string& file_name,
            const std::string& username,
            const std::vector<unsigned char>& data);
        static FileTransferInfo prepare_for_download(
            const std::string& file_name,
            const std::string& username, 
            size_t file_size);
    };
    extern std::mutex file_transfers_mutex;
    extern std::map<std::string,FileTransferInfo> file_transfers;
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