#include "file.hpp"
#include <string>
#include <fstream>
#include <vector>
#include <mutex>
#include <map>
#include "../../defines.hpp"
#include "../../ansi_escape.hpp"
#include "../MessageQueue/MessageQueue.hpp"
#include "../../logging/logging.hpp"
#include "../udp/udp.hpp"
#include "../../multithreading/multithreading.hpp"
#include "../../parsing/parsing.hpp"
#include "../../terminal/terminal.hpp"
namespace network::file
{
    MessageQueue file_queue;
    #define WINDOW_SIZE 512
    enum class FileTransferDirection
    {
        upload,
        download
    };
    struct FileTransferInfo
    {
        std::string file_name;
        std::string username;
        std::vector<unsigned char> data;
        unsigned long long sequence_number = 0;
        FileTransferDirection direction;
    };
    std::mutex file_transfers_mutex;
    std::map<std::string,FileTransferInfo> file_transfers;
    void file()
    {
        while(true)
        {
            auto item = file_queue.pull();
            auto args = parsing::msg_split(item.msg);
            // FILE <base64 file hash> <sequence number> <base64 data>
            if(args[0] == "FILE" and args.size() == 4)
            {

            }
            // FILEACK <base64 file hash> <next sequence number to receive>
            else if(args[0] == "FILEACK" and args.size() == 3)
            {

            }
            // FILEINIT <base64 file hash> <total file size> <file name>
            else if(args[0] == "FILEINIT" and args.size() == 4)
            {
                try{
                    auto file_size = std::stoull(args[2]);
                    std::string file_name = parsing::clean_file_name(args[3]);
                    terminal::input("Accept a file of " HIGHLIGHT + std::to_string(file_size/1024) + RESET "KB from " HIGHLIGHT + item.src + RESET "? (y/n)",
                    [item,file_size,args](const std::string& input){
                        if(input == "Y" or input == "y")
                        {
                            //accept_file();
                        }else
                        {
                            logging::log("MSG","File refused from \"" HIGHLIGHT +item.src+ RESET "\"");
                            network::udp::send(parsing::compose_message({"FILESTOP",args[1]}),item.src_endpoint);
                        }
                    });
                }catch(std::exception&)
                {}
            }
            // FILESTOP <base64 file hash>
            else if(args[0] == "FILESTOP" and args.size() == 2)
            {

            }
            else 
            {
                logging::log("DBG","Dropped " HIGHLIGHT + args[0] + RESET " from " HIGHLIGHT + item.src_endpoint.address().to_string() + RESET ":" HIGHLIGHT + std::to_string(item.src_endpoint.port()) + RESET);
            }
        }
    }
    void init()
    {
        udp::register_queue("FILE",file_queue,true);
        udp::register_queue("FILEACK",file_queue,true);
        udp::register_queue("FILEINIT",file_queue,true);
        udp::register_queue("FILESTOP",file_queue,true);
        multithreading::add_service("file",file);
    }
}