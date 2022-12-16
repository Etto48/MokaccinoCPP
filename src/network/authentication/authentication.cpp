#include "authentication.hpp"
#include "../../defines.hpp"
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/buffer.h> 
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <exception>
#include <filesystem>
#include <map>
#include "../../logging/logging.hpp"
#include "../../base64/base64.h"
#include "KnownUsers/KnownUsers.hpp"
namespace network::authentication
{  
    #define CONCAT_CHAR ':'
    RSA* local_key = nullptr;
    KnownUsers known_users;
    std::string pubkey_to_string(const RSA* r)
    {
        auto* n = RSA_get0_n(r);
        auto* e = RSA_get0_e(r);
        unsigned n_len = BN_num_bytes(n);
        unsigned e_len = BN_num_bytes(e);
        std::unique_ptr<unsigned char> n_ptr{new unsigned char[n_len]};
        std::unique_ptr<unsigned char> e_ptr{new unsigned char[e_len]};
        BN_bn2bin(n,n_ptr.get());
        BN_bn2bin(e,e_ptr.get());
        std::unique_ptr<char> n_str{new char[b64e_size(n_len)+1]};
        std::unique_ptr<char> e_str{new char[b64e_size(e_len)+1]};
        b64_encode(n_ptr.get(),n_len,(unsigned char*)n_str.get());
        b64_encode(e_ptr.get(),e_len,(unsigned char*)e_str.get());
        return std::string(n_str.get())+CONCAT_CHAR+std::string(e_str.get());
    }
    RSA* string_to_pubkey(const std::string& s)
    {
        auto split_at = s.find(CONCAT_CHAR);
        assert(split_at != s.npos);
        auto n_str = s.substr(0,split_at);
        auto e_str = s.substr(split_at+1);
        auto n_len = b64d_size((unsigned int)n_str.length());
        auto e_len = b64d_size((unsigned int)e_str.length());
        std::unique_ptr<unsigned char> n_ptr{new unsigned char[n_len]};
        std::unique_ptr<unsigned char> e_ptr{new unsigned char[e_len]};
        b64_decode((unsigned char*)n_str.c_str(),(unsigned int)n_str.length(),n_ptr.get());
        b64_decode((unsigned char*)e_str.c_str(),(unsigned int)e_str.length(),e_ptr.get());
        RSA* ret = RSA_new();
        BIGNUM* n = BN_bin2bn(n_ptr.get(),n_len,nullptr);
        BIGNUM* e = BN_bin2bn(e_ptr.get(),e_len,nullptr);
        RSA_set0_key(ret,n,e,nullptr);
        return ret;
    }
    void print_keys()
    {
        auto* n = RSA_get0_n(local_key);
        auto* d = RSA_get0_d(local_key);
        auto* e = RSA_get0_e(local_key);
        unsigned n_len = BN_num_bytes(n);
        unsigned d_len = BN_num_bytes(d);
        unsigned e_len = BN_num_bytes(e);
        std::unique_ptr<unsigned char> n_ptr{new unsigned char[n_len]};
        std::unique_ptr<unsigned char> d_ptr{new unsigned char[d_len]};
        std::unique_ptr<unsigned char> e_ptr{new unsigned char[e_len]};
        BN_bn2bin(n,n_ptr.get());
        BN_bn2bin(d,d_ptr.get());
        BN_bn2bin(e,e_ptr.get());
        std::unique_ptr<char> n_str{new char[b64e_size(n_len)+1]};
        std::unique_ptr<char> d_str{new char[b64e_size(d_len)+1]};
        std::unique_ptr<char> e_str{new char[b64e_size(e_len)+1]};
        b64_encode(n_ptr.get(),n_len,(unsigned char*)n_str.get());
        b64_encode(d_ptr.get(),d_len,(unsigned char*)d_str.get());
        b64_encode(e_ptr.get(),e_len,(unsigned char*)e_str.get());
        logging::log("DBG",std::string("n: ")+n_str.get());
        logging::log("DBG",std::string("d: ")+d_str.get());
        logging::log("DBG",std::string("e: ")+e_str.get());
    }
    void gen_and_load_keys(int bits = 2048)
    {
        BIO *bp_public = nullptr;
        BIO *bp_private = nullptr;
        BIGNUM *bne = nullptr;
        unsigned long long e = RSA_F4;
        try
        {
            logging::log("DBG","Generating RSA keypair");
            bne = BN_new();
            if(BN_set_word(bne,e) != 1)
                throw std::exception{};

            local_key = RSA_new();
            if(RSA_generate_key_ex(local_key,bits,bne,nullptr) != 1)
                throw std::exception{};
            
            bp_public = BIO_new_file(PUBKEY_PATH.c_str(),"w+");
            bp_private = BIO_new_file(PRIVKEY_PATH.c_str(),"w+");
            if(PEM_write_bio_RSAPrivateKey(bp_private,local_key,nullptr,nullptr,0,nullptr,nullptr) != 1)
                throw std::exception{};
            if(PEM_write_bio_RSAPublicKey(bp_public,local_key) != 1)
                throw std::exception{};

            logging::log("DBG","RSA keypair correctly generated and saved");
        }catch(std::exception&)
        {//free and error
            auto code = ERR_get_error();
            BIO_free_all(bp_public);
            BIO_free_all(bp_private);
            BN_free(bne);
            std::unique_ptr<char> error_string{new char[256]};
            ERR_error_string(code,error_string.get());
            logging::log("ERR","Error generating RSA keypair: " + std::string(error_string.get()));
        }
        BIO_free_all(bp_public);
        BIO_free_all(bp_private);
        BN_free(bne);
    }
    void init()
    {
        BIO *bp_public = nullptr;
        BIO *bp_private = nullptr;
        BIGNUM *bne = nullptr;
        if(not std::filesystem::is_regular_file(PUBKEY_PATH) or not std::filesystem::is_regular_file(PRIVKEY_PATH))
        {//gen key
            gen_and_load_keys(3072);
        }else
        {//load keys
            try
            {    
                bp_public = BIO_new_file(PUBKEY_PATH.c_str(),"r");
                bp_private = BIO_new_file(PRIVKEY_PATH.c_str(),"r");
                if((local_key = PEM_read_bio_RSAPublicKey(bp_public,&local_key,nullptr,nullptr)) == nullptr)
                    throw std::exception{};
                if((local_key = PEM_read_bio_RSAPrivateKey(bp_private,&local_key,nullptr,nullptr)) == nullptr)
                    throw std::exception{};
                logging::log("DBG","RSA keypair correctly loaded");
            }catch(std::exception&)
            {
                auto code = ERR_get_error();
                BIO_free_all(bp_public);
                BIO_free_all(bp_private);
                RSA_free(local_key);
                std::unique_ptr<char> error_string{new char[256]};
                ERR_error_string(code,error_string.get());
                logging::log("ERR","Error loading RSA keypair: " + std::string(error_string.get()));
            }
            BIO_free_all(bp_public);
            BIO_free_all(bp_private);
        }
        //print_keys();
        //logging::log("DBG","LPK: "+local_public_key());
        known_users.load(KNOWN_USERS_FILE);
        known_users.add_key("loopback",local_public_key());
    }

    bool verify(const std::string& data, const std::string& signed_data, const std::string& username)
    {
        try{
            auto key_str = known_users.get_key(username);
            std::unique_ptr<RSA,decltype(&::RSA_free)> pubkey {string_to_pubkey(key_str),RSA_free};
            std::unique_ptr<unsigned char> hashed_data{new unsigned char[SHA256_DIGEST_LENGTH]};
            SHA256((unsigned char*)data.c_str(),data.length(),hashed_data.get());
            std::unique_ptr<unsigned char> decoded_signature{new unsigned char[b64d_size((unsigned int)signed_data.length())]};
            unsigned int signature_len = b64_decode((unsigned char*)signed_data.c_str(),(unsigned int)signed_data.length(),decoded_signature.get());
            bool success = (RSA_verify(NID_sha256,hashed_data.get(),SHA256_DIGEST_LENGTH,decoded_signature.get(),signature_len,pubkey.get()) == 1);
            return success;
        }catch(KnownUsers::KeyNotFound&)
        {
            return false;
        }
    }
    std::string sign(const std::string& data)
    {
        std::unique_ptr<unsigned char> hashed_data{new unsigned char[SHA256_DIGEST_LENGTH]};
        SHA256((unsigned char*)data.c_str(),data.length(),hashed_data.get());
        std::unique_ptr<unsigned char> signed_data{new unsigned char[RSA_size(local_key)]};
        unsigned int siglen = 0;
        RSA_sign(NID_sha256,hashed_data.get(),SHA256_DIGEST_LENGTH,signed_data.get(),&siglen,local_key);
        std::unique_ptr<char> sign_b64{new char[b64e_size(siglen)+1]};
        b64_encode(signed_data.get(),siglen,(unsigned char*)sign_b64.get());
        return std::string(sign_b64.get());
    }
    std::string local_public_key()
    {
        return pubkey_to_string(local_key);
    }
    std::string generate_nonce()
    {
        std::unique_ptr<unsigned char> random_bytes{new unsigned char[NONCE_NON_ENCODED_LENGTH]};
        RAND_bytes(random_bytes.get(),NONCE_NON_ENCODED_LENGTH);
        std::unique_ptr<char> encoded_bytes{new char[b64e_size(NONCE_NON_ENCODED_LENGTH)+1]};
        b64_encode(random_bytes.get(),NONCE_NON_ENCODED_LENGTH,(unsigned char*)encoded_bytes.get());
        return std::string(encoded_bytes.get());
    }
}