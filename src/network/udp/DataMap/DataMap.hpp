#pragma once
#include "../../../defines.hpp"
#include <map>
#include <mutex>
#include <string>
#include <boost/asio/ip/udp.hpp>
#include "../../../logging/logging.hpp"

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
        std::mutex obj;
        std::string server_name;
    public:
        
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
    private:
        bool _add_user(const std::string& name,const boost::asio::ip::udp::endpoint& endpoint);
        bool _remove_user(const std::string& name);
        bool _remove_user(const boost::asio::ip::udp::endpoint& endpoint);

        PeerData& _op_sqbr(const std::string& name);
        PeerData& _op_sqbr(const boost::asio::ip::udp::endpoint& endpoint);
    };
}