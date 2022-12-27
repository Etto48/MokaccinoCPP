#pragma once
#include <string>
#include "KnownUsers/KnownUsers.hpp"
namespace network::authentication
{
    extern KnownUsers known_users;
    /**
     * @brief validate if the selected user correctly signed the data sent to him
     * the function checks if data == D(signed_data,pubkeys[username])
     * 
     * @param data data to sign
     * @param signed_data signed data
     * @param username user that signed the data
     * @return true if the user signed the data
     * @return false if the user did not sign the data
     */
    bool verify(const std::string& data, const std::string& signed_data, const std::string& username);
    /**
     * @brief sign the data with the local private key
     * 
     * @param data data to sign
     * @return signed data
     */
    std::string sign(const std::string& data);
    /**
     * @brief returns the local public key in a string in PEM format without the starting and ending line
     * 
     * @return the public key in string format, n and e are encoded with base64
     */
    std::string local_public_key();
    /**
     * @brief generate a random string
     * 
     * @return the random string
     */
    std::string generate_nonce();
    /**
     * @brief initialize the module
     * 
     */
    void init();
}