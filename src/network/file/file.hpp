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
    /**
     * @brief this struct stores the info about an ongoing file transfer
     * 
     */
    struct FileTransferInfo
    {
        // file name (without dirs)
        std::string file_name;
        // user with who you are exchanging the file
        std::string username;
        // true if the other user accepted the transfer
        bool accepted;
        // file binary data (bytes)
        std::vector<unsigned char> data;
        //true if a chunk has been received (used only for download)
        std::vector<bool> received_chunks;
        // first byte not received (only for download) or first byte yet to send (only for upload)
        size_t next_sequence_number = 0;
        // last ack sent/received (up/down)
        size_t last_acked_number = 0;
        // FileTransferDirection::upload or FileTransferDirection::download
        FileTransferDirection direction;
        // time of the last sent/received ack
        boost::posix_time::ptime last_ack;
        // get how many chunks there are in a file of "data_size" bytes
        static size_t chunk_count(size_t data_size);
        // constructor for upload
        static FileTransferInfo prepare_for_upload(
            const std::string& file_name,
            const std::string& username,
            const std::vector<unsigned char>& data);
        // constructore for download
        static FileTransferInfo prepare_for_download(
            const std::string& file_name,
            const std::string& username, 
            size_t file_size);
    };
    /**
     * @brief use this before accessing file_transfers
     * 
     */
    extern std::mutex file_transfers_mutex;
    /**
     * @brief lock file_transfers_mutex before accessing, this map contains every info about ongoing file transfers,
     * the key is the file hash encoded base64
     * 
     */
    extern std::map<std::string,FileTransferInfo> file_transfers;
    /**
     * @brief get how many files are being transferred
     * 
     * @return file transfers count
     */
    size_t ongoing_transfers_count();
    /**
     * @brief thrown by init_file_upload when the file provided can't be opened
     * 
     */
    class FileNotFound: public std::exception
    {
    public:
        const char* what(){return "FileNotFound";}
    };
    /**
     * @brief thrown by init_file_upload when the selected user is not connected
     * 
     */
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