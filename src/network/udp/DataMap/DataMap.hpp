#pragma once
#include <map>
#include <mutex>
#include <string>
#include <exception>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/ip/udp.hpp>
#include "../crypto/crypto.hpp"

namespace network
{
    /**
     * @brief this class is used for udp::connection_map
     * this can be used to store data before it is completely received, for checking
     * if a user is registered and what username corresponds to an endpoint
     * 
     */
    class DataMap
    {
    public:
        /**
         * @brief structure contained in name_data map and is returned from the operator[]
         * 
         */
        struct PeerData
        {
            std::string name;
            boost::asio::ip::udp::endpoint endpoint;
            std::string tmpData;
            boost::posix_time::ptime ping_sent;
            unsigned short last_ping_id = 0;
            boost::posix_time::time_duration last_latency;
            boost::posix_time::time_duration avg_latency;
            unsigned short offline_strikes = 0;
            bool encrypted = false;
            std::string asymmetric_key;
            udp::crypto::Key symmetric_key;
        };
    private:
        /**
         * @brief this maps an endpoint to a username
         * 
         */
        std::map<boost::asio::ip::udp::endpoint,std::string> endpoint_name;
        /**
         * @brief this maps a username to the data
         * 
         */
        std::map<std::string,PeerData> name_data; 
        std::string server_name;
    public:
        /**
         * @brief you must acquire this before using the operator[]
         * 
         */
        std::recursive_mutex obj;
        
        /**
         * @brief add a new user
         * 
         * @param name username
         * @param endpoint it's ip and port
         * @return true if the user was added correctly
         * @return false if the user was already present
         */
        bool add_user(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint);
        /**
         * @brief remove a user
         * 
         * @param name username
         * @return true if the user was removed correcly
         * @return false if the user was not present
         */
        bool remove_user(const std::string& name);
        /**
         * @brief remove a user
         * 
         * @param endpoint it's ip and port
         * @return true if the user was removed correcly
         * @return false if the user was not present
         */
        bool remove_user(const boost::asio::ip::udp::endpoint& endpoint);
        /**
         * @brief get how many users are registered
         * 
         * @return how many users are registered
         */
        size_t size();
        /**
         * @brief this exception is thrown by operator[] when the user is not found
         * 
         */
        class NotFound: public std::exception
        {
        public:
            const char* what();
        };

        /**
         * @brief retrieve the requested PeerData struct, if the search fails throws NotFound
         * 
         * @param name search by name
         * @return the selected PeerData
         */
        PeerData& operator[](const std::string& name);
        /**
         * @brief retrieve the requested PeerData struct, if the search fails throws NotFound
         * 
         * @param name search by endpoint
         * @return the selected PeerData
         */
        PeerData& operator[](const boost::asio::ip::udp::endpoint& endpoint);

        /**
         * @brief check if a user is logged in
         * 
         * @param name username
         * @return true if it's logged in
         * @return false if it's not logged in
         */
        bool check_user(const std::string& name);
        /**
         * @brief check if a user is logged in
         * 
         * @param name user ip and port
         * @return true if it's logged in
         * @return false if it's not logged in
         */
        bool check_user(const boost::asio::ip::udp::endpoint& endpoint);
        /**
         * @brief get the first non local peer
         * 
         * @return the endpoint of the peer
         */
        boost::asio::ip::udp::endpoint server();
        /**
         * @brief return a vector of connected users and their ip and port
         * 
         * @return a vector containing pairs of username, endpoint
         */
        std::vector<std::pair<std::string,boost::asio::ip::udp::endpoint>> get_connected_users();
    private:
        bool _add_user(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint);
        bool _remove_user(const std::string& name);
        bool _remove_user(const boost::asio::ip::udp::endpoint& endpoint);

        PeerData& _op_sqbr(const std::string& name);
        PeerData& _op_sqbr(const boost::asio::ip::udp::endpoint& endpoint);
    };
}