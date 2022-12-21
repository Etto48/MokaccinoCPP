#include "file.hpp"
#include "../../defines.hpp"
#include "../../ansi_escape.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <openssl/sha.h>
#include <boost/thread.hpp>
#include "../MessageQueue/MessageQueue.hpp"
#include "../../logging/logging.hpp"
#include "../udp/DataMap/DataMap.hpp"
#include "../udp/udp.hpp"
#include "../../multithreading/multithreading.hpp"
#include "../../parsing/parsing.hpp"
#include "../../terminal/terminal.hpp"
#include "../../base64/base64.h"
namespace network::file
{
    MessageQueue file_queue;
    constexpr size_t CHUNK_SIZE = 512;
    constexpr unsigned int ACK_EVERY = 100;//milliseconds if no new packets received
    constexpr unsigned int ACCEPT_TIMEOUT = 10; // seconds
    size_t window_size = 10; // CHUNKS
    size_t FileTransferInfo::chunk_count(size_t data_size)
    {
        size_t ret = data_size/CHUNK_SIZE;
        if(data_size%CHUNK_SIZE != 0)
            ret++;
        return ret;
    }
    FileTransferInfo FileTransferInfo::prepare_for_upload(
            const std::string& file_name,
            const std::string& username,
            const std::vector<unsigned char>& data)
    {
        FileTransferInfo ret;
        ret.file_name = file_name;
        ret.username = username;
        ret.accepted = false;
        ret.data = data;
        ret.received_chunks = std::vector<bool>(chunk_count(data.size()),false);
        ret.next_sequence_number = 0;
        ret.last_acked_number = 0;
        ret.direction = FileTransferDirection::upload;
        ret.last_ack = boost::posix_time::microsec_clock::local_time();
        return ret;
    }
    FileTransferInfo FileTransferInfo::prepare_for_download(
        const std::string& file_name, 
        const std::string& username, 
        size_t file_size)
    {
        FileTransferInfo ret;
        ret.file_name = file_name;
        ret.username = username;
        ret.accepted = true;
        ret.data = std::vector<unsigned char>(file_size);
        ret.received_chunks = std::vector<bool>(chunk_count(file_size),false);
        ret.next_sequence_number = 0;
        ret.last_acked_number = 0;
        ret.direction = FileTransferDirection::download;
        ret.last_ack = boost::posix_time::microsec_clock::local_time();
        return ret;
    }
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
            throw UserNotFound{};
        std::fstream f{filename,std::ios::in|std::ios::binary};
        if(not f)
            throw FileNotFound{};
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
        udp::send(parsing::compose_message({"FILEINIT",file_hash,std::to_string(info.data.size()),info.file_name}),to);
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
        udp::send(parsing::compose_message({"FILEACK",file_hash,"0"}),from);
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
        if(info.direction != FileTransferDirection::download)
            return false;
        if(sequence_number%CHUNK_SIZE!=0)
            return false;
        std::unique_ptr<unsigned char> chunk{new unsigned char[b64d_size((unsigned int)data.length())]};
        auto data_size = b64_decode((unsigned char*)data.c_str(),(unsigned int)data.length(),chunk.get());
        auto chunk_number = sequence_number/CHUNK_SIZE;
        if(info.received_chunks[chunk_number])
        {// already received
            udp::send(parsing::compose_message({"FILEACK",file_hash,std::to_string(info.next_sequence_number)}),endpoint);
            info.last_ack = boost::posix_time::microsec_clock::local_time();
            info.last_acked_number = info.next_sequence_number;
            return true;
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
        // check if we haven't acked for window_size chunks or we have received the entire file or we have a duplicate file packet
        if(info.next_sequence_number == info.data.size() or info.last_acked_number+(window_size*CHUNK_SIZE) <= info.next_sequence_number)
        { // in this case we need to send an ACK asap
            udp::send(parsing::compose_message({"FILEACK",file_hash,std::to_string(info.next_sequence_number)}),endpoint);
            info.last_ack = boost::posix_time::microsec_clock::local_time();
            info.last_acked_number = info.next_sequence_number;

            if(info.next_sequence_number == info.data.size())
                return _finalize_file_download(file_hash);
        }
        return true;
    }
    // returns the size of the data sent
    inline size_t _create_and_send_file_packet(size_t sequence_number_to_send,const std::string& file_hash, const FileTransferInfo& info, const boost::asio::ip::udp::endpoint& endpoint)
    {
        auto packet_size = std::min(CHUNK_SIZE,info.data.size() - sequence_number_to_send);
        if(packet_size == 0)
            return 0;
        std::unique_ptr<char> encoded_data{new char[b64e_size((unsigned int)packet_size)+1]};
        b64_encode(info.data.data()+sequence_number_to_send,(unsigned int)packet_size,(unsigned char*)encoded_data.get());
        udp::send(parsing::compose_message({"FILE",file_hash,std::to_string(sequence_number_to_send),encoded_data.get()}),endpoint);
        return packet_size;
    }
    bool handle_ack(const std::string& username, const boost::asio::ip::udp::endpoint& endpoint, const std::string& file_hash, size_t acked_number)
    {
        std::unique_lock lock(file_transfers_mutex);
        if(file_transfers.find(file_hash) == file_transfers.end())
            return false;
        auto& info = file_transfers[file_hash];
        if(info.username != username)
            return false;
        if(info.direction != FileTransferDirection::upload)
            return false;
        if(acked_number != info.data.size() and acked_number%CHUNK_SIZE != 0)
            return false;
        if(acked_number > info.next_sequence_number)//acked a packet we did not send
            return false;
        info.accepted = true;

        if(acked_number == info.last_acked_number)//duplicate ack
        {// we need to resend the double acked packet because it was lost
            _create_and_send_file_packet(info.last_acked_number,file_hash,info,endpoint);
            return true;
        }
        info.last_acked_number = acked_number;
        info.last_ack = boost::posix_time::microsec_clock::local_time();
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
            std::vector<std::string> keys;
            {
                std::unique_lock lock(file_transfers_mutex);
                keys.reserve(file_transfers.size());
                for(auto& [key, value]: file_transfers)
                {
                    keys.emplace_back(key);
                }
            }
            if(keys.empty())
                boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            else
            {
                for(auto& k: keys)
                {
                    std::unique_lock lock(file_transfers_mutex);
                    if(file_transfers.find(k) != file_transfers.end())
                    { // key still present
                        auto& info = file_transfers[k];
                        try
                        {
                            auto endpoint = udp::connection_map[info.username].endpoint;   
                            auto window_difference = info.next_sequence_number - info.last_acked_number;
                            switch (info.direction)
                            {
                            case FileTransferDirection::upload:
                                // we have "window_difference" packets that we don't know if they were received or not
                                // we send (window_size*CHUNK_SIZE)-window_difference bytes starting from next_sequence_number
                                // so that we have the next window_difference == window_size*CHUNK_SIZE
                                if(info.accepted)
                                {
                                    if(window_difference < window_size*CHUNK_SIZE)
                                    {
                                        for(;window_difference < window_size*CHUNK_SIZE;window_difference = info.next_sequence_number - info.last_acked_number)
                                        {
                                            auto packet_size = _create_and_send_file_packet(info.next_sequence_number,k,info,endpoint);
                                            if(packet_size == 0)
                                                break;//we sent the whole file
                                            info.next_sequence_number += packet_size;
                                        }
                                    }
                                    /*else
                                    {//we have sent a whole window_size and not received a single ack
                                        // we do nothing and wait for a duplicate ack
                                    }*/
                                }
                                else
                                {// the user did not accept/went offline
                                    auto now = boost::posix_time::microsec_clock::local_time();
                                    if((now-info.last_ack).total_seconds() > ACCEPT_TIMEOUT)
                                    {
                                        udp::send(parsing::compose_message({"FILESTOP",k}),info.username);
                                        logging::log("MSG","Request to send a file to " HIGHLIGHT +info.username+ RESET " timed out");
                                        file_transfers.erase(k);
                                    }
                                }
                                break;
                            case FileTransferDirection::download:
                                {
                                    auto now = boost::posix_time::microsec_clock::local_time();
                                    // we have "window_difference" packets that we didn't ack or the last ack was very long ago
                                    if(window_difference != 0 or (now-info.last_ack).total_milliseconds() > ACK_EVERY)
                                    { // we ack everything we received
                                        info.last_acked_number = info.next_sequence_number;
                                        udp::send(parsing::compose_message({"FILEACK",k,std::to_string(info.last_acked_number)}),endpoint);
                                    }
                                }
                                break;
                            }
                        }
                        catch(DataMap::NotFound&)
                        { // user disconnected during file transfer
                            logging::log("ERR","File transfer with " HIGHLIGHT + info.username + RESET " stopped because the user disconnected");
                            std::unique_lock lock(file_transfers_mutex);
                            file_transfers.erase(k);
                        }
                    }
                }
                boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
            }
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
                    handle_ack(item.src,item.src_endpoint,args[1],next_sequence_number);
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
    size_t ongoing_transfers_count()
    {
        std::unique_lock lock(file_transfers_mutex);
        return file_transfers.size();
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