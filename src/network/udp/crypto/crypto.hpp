#pragma once
#include <tuple>
#include <string>
#include <memory>
#include <openssl/sha.h>
#include <openssl/evp.h>
namespace network::udp::crypto
{
    struct Key
    {
        unsigned char data[SHA256_DIGEST_LENGTH] = {0};
    };
    std::string pubkey_to_string(EVP_PKEY* x);
    std::unique_ptr<EVP_PKEY,decltype(&::EVP_PKEY_free)> string_to_pubkey(const std::string& s);

    std::string privkey_to_string(EVP_PKEY* x);
    std::unique_ptr<EVP_PKEY,decltype(&::EVP_PKEY_free)> string_to_privkey(const std::string& s);

    std::unique_ptr<EVP_PKEY,decltype(&::EVP_PKEY_free)> gen_ecdhe_key();
    Key ecdhe(EVP_PKEY* local_private, EVP_PKEY* remote_public);
    std::tuple<std::string,std::string,std::string> encrypt(const std::string& message, const Key& key);
    std::string decrypt(const std::string& cryptogram, const Key& key, const std::string& iv, const std::string& tag);
}
