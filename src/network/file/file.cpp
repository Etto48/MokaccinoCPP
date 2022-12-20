#include "file.hpp"
#include "../../defines.hpp"
#include "../../ansi_escape.hpp"
#include <string>
#include <fstream>
#include <vector>
#include <mutex>
#include <filesystem>
#include <map>
#include <algorithm>
#include <openssl/sha.h>
#include <boost/thread.hpp>
#include "../MessageQueue/MessageQueue.hpp"
#include "../../logging/logging.hpp"
#include "../udp/udp.hpp"
#include "../../multithreading/multithreading.hpp"
#include "../../parsing/parsing.hpp"
#include "../../terminal/terminal.hpp"
#include "../../base64/base64.h"
namespace network::file
{
    MessageQueue file_queue;
    #define CHUNK_SIZE 512
    size_t window_size = 10; // CHUNKS
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
        std::vector<bool> received_chunks;
        size_t sent_sequence_number = 0;
        // if direction is receive you will only use this
        size_t next_sequence_number = 0;
        size_t last_acked_number = 0;
        FileTransferDirection direction;
        boost::posix_time::ptime last_ack;
        static size_t chunk_count(size_t data_size)
        {
            size_t ret = data_size/CHUNK_SIZE;
            if(data_size%CHUNK_SIZE != 0)
                ret++;
            return ret;
        }
        static FileTransferInfo prepare_for_upload(
            const std::string& file_name,
            const std::string& username,
            const std::vector<unsigned char>& data)
        {
            FileTransferInfo ret;
            ret.file_name = file_name;
            ret.username = username;
            ret.data = data;
            ret.received_chunks = std::vector<bool>(chunk_count(data.size()),false);
            ret.sent_sequence_number = 0;
            ret.next_sequence_number = 0;
            ret.last_acked_number = 0;
            ret.direction = FileTransferDirection::upload;
            ret.last_ack = boost::posix_time::microsec_clock().local_time();
            return ret;
        }
        static FileTransferInfo prepare_for_download(const std::string& file_name, const std::string& username, size_t file_size)
        {
            FileTransferInfo ret;
            ret.file_name = file_name;
            ret.username = username;
            ret.data = std::vector<unsigned char>(file_size);
            ret.received_chunks = std::vector<bool>(chunk_count(file_size),false);
            ret.sent_sequence_number = 0;
            ret.next_sequence_number = 0;
            ret.last_acked_number = 0;
            ret.direction = FileTransferDirection::download;
            ret.last_ack = boost::posix_time::microsec_clock().local_time();
            return ret;
        }
    };
    std::mutex file_transfers_mutex;
    std::map<std::string,FileTransferInfo> file_transfers;
    std::string hash(const std::vector<unsigned char>& file_content)
    {
        std::unique_ptr<unsigned char> md{new unsigned char[SHA256_DIGEST_LENGTH]};
        std::unique_ptr<char> b64md{new char[b64e_size(SHA256_DIGEST_LENGTH)+1]};
        SHA256(file_content.data(),file_content.size(),md.get());
        b64_encode(md.get(),SHA256_DIGEST_LENGTH,(unsigned char*)b64md.get());
        return b64md.get();
    }
    bool init_file_upload(const std::string& to, const std::string& filename)
    {
        if(not udp::connection_map.check_user(to))
            return false;
        std::fstream f{filename,std::ios::in|std::ios::binary};
        if(not f)
            return false;
        f.seekg(0,std::ios::end);
        auto file_size = f.tellg();
        f.seekg(0,std::ios::beg);
        std::vector<unsigned char> file_data(file_size);
        f.read((char*)file_data.data(),file_size);
        FileTransferInfo info = FileTransferInfo::prepare_for_upload(
            std::filesystem::path(filename).filename().string(),
            to,
            file_data
        );
        auto file_hash = hash(file_data);
        std::unique_lock lock(file_transfers_mutex);
        if(file_transfers.find(file_hash) != file_transfers.end())
            return false;
        file_transfers[file_hash] = info;
        return true;
    }
    bool init_file_download(const std::string& from, const std::string& file_hash, size_t file_size, const std::string& file_name)
    {
        FileTransferInfo info = FileTransferInfo::prepare_for_download(
            parsing::clean_file_name(file_name),
            from,
            file_size
        );
        std::unique_lock lock(file_transfers_mutex);
        if(file_transfers.find(file_hash) != file_transfers.end())
            return false;
        file_transfers[file_hash] = info;
        return true;
    }
    bool _finalize_file_download(const std::string& file_hash)
    {
        if(file_transfers.find(file_hash) == file_transfers.end())
            return false;
        auto& info = file_transfers[file_hash];
        if(info.next_sequence_number != info.data.size())
            return false;
        std::filesystem::create_directories(DOWNLOAD_PATH);
        std::fstream f{DOWNLOAD_PATH+info.file_name,std::ios::out|std::ios::trunc|std::ios::binary};
        f.write((char*)info.data.data(),info.data.size());
        logging::log("MSG","File from " HIGHLIGHT + info.username + RESET " saved at \"" HIGHLIGHT +DOWNLOAD_PATH+info.file_name+ RESET "\"");
        file_transfers.erase(file_hash);
        return true;
    }
    bool handle_data(const std::string& username,const boost::asio::ip::udp::endpoint& endpoint,const std::string& file_hash, size_t sequence_number, const std::string& data)
    {
        std::unique_lock lock(file_transfers_mutex);
        if(file_transfers.find(file_hash) == file_transfers.end())
            return false;
        auto info = file_transfers[file_hash];
        if(info.username != username)
            return false;
        if(sequence_number%CHUNK_SIZE!=0)
            return false;
        std::unique_ptr<unsigned char> chunk{new unsigned char[b64d_size((unsigned int)data.length())]};
        auto data_size = b64_decode((unsigned char*)data.c_str(),(unsigned int)data.length(),chunk.get());
        auto chunk_number = sequence_number/CHUNK_SIZE;
        if(info.received_chunks[chunk_number])
        {// already received
            return false;
        }
        //not of CHUNK_SIZE or not last chunk and missing size
        if(data_size != CHUNK_SIZE and (chunk_number != info.received_chunks.size() - 1 or data_size != info.data.size()%CHUNK_SIZE))
            return false;
        std::copy(chunk.get(),chunk.get()+data_size,info.data.begin()+sequence_number);
        info.received_chunks[chunk_number] = true;
        //check if there are chunks received out of order
        for(size_t i = info.next_sequence_number/CHUNK_SIZE; info.received_chunks[i]; i++)
        {
            if(i == info.received_chunks.size()-1) // last chunk
                info.next_sequence_number+=info.data.size()%CHUNK_SIZE;
            else // any other chunk
                info.next_sequence_number+=CHUNK_SIZE;
        }
        // check if we haven't acked for window_size chunks or we have received the entire file
        if(info.next_sequence_number == info.data.size() or info.last_acked_number+(window_size*CHUNK_SIZE) <= info.next_sequence_number)
        { // in this case we need to send an ACK asap
            udp::send(parsing::compose_message({"FILEACK",file_hash,std::to_string(info.next_sequence_number)}),endpoint);
            info.last_ack = boost::posix_time::microsec_clock().local_time();
            info.last_acked_number = info.next_sequence_number;

            if(info.next_sequence_number == info.data.size())
                return _finalize_file_download(file_hash);
        }
        return true;
    }
    bool handle_ack(const std::string& username, const std::string& file_hash, size_t next_sequence_number)
    {
        std::unique_lock lock(file_transfers_mutex);
        if(file_transfers.find(file_hash) == file_transfers.end())
            return false;
        auto& info = file_transfers[file_hash];
        if(info.username != username)
            return false;
        if(next_sequence_number != info.data.size() and next_sequence_number%CHUNK_SIZE != 0)
            return false;
        if(next_sequence_number == info.next_sequence_number)//duplicate ack
            return false;
        info.next_sequence_number = next_sequence_number;
        info.last_ack = boost::posix_time::microsec_clock().local_time();
        return true;
    }
    bool delete_file_transfer(const std::string& username, const std::string& file_hash)
    {
        std::unique_lock lock(file_transfers_mutex);
        if(file_transfers.find(file_hash) == file_transfers.end())
            return false;
        auto& info = file_transfers[file_hash];
        if(info.username != username)
            return false;
        file_transfers.erase(file_hash);
        return true;
    }
    void file_sender()
    {
        while(true)
        {
            bool empty = false;
            {
                std::unique_lock lock(file_transfers_mutex);
                if(file_transfers.empty())
                    empty = true;
            }
            if(empty)
                boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            // TODO: finish

        }
    }
    void file()
    {
        while(true)
        {
            auto item = file_queue.pull();
            auto args = parsing::msg_split(item.msg);
            // FILE <base64 file hash> <sequence number> <base64 data>
            if(args[0] == "FILE" and args.size() == 4)
            {
                try
                {
                    auto sequence_number = std::stoull(args[2]);
                    handle_data(item.src,item.src_endpoint,args[1],sequence_number,args[3]);
                }catch(std::exception&)
                {}
            }
            // FILEACK <base64 file hash> <next sequence number to receive>
            else if(args[0] == "FILEACK" and args.size() == 3)
            {
                try{
                    auto next_sequence_number = std::stoull(args[2]);
                    handle_ack(item.src,args[1],next_sequence_number);
                }catch(std::exception&)
                {}
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
                            if(not init_file_download(item.src,args[1],file_size,args[3]))
                                logging::log("ERR","Error downloading a file from " HIGHLIGHT + item.src + RESET);
                        }else
                        {
                            logging::log("MSG","File refused from \"" HIGHLIGHT +item.src+ RESET "\"");
                            udp::send(parsing::compose_message({"FILESTOP",args[1]}),item.src_endpoint);
                        }
                    });
                }catch(std::exception&)
                {}
            }
            // FILESTOP <base64 file hash>
            else if(args[0] == "FILESTOP" and args.size() == 2)
            {
                if(delete_file_transfer(item.src,args[1]))
                    logging::log("MSG","File transfer stopped from " HIGHLIGHT + item.src + RESET);
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
        multithreading::add_service("file_sender",file_sender);
    }
}