#include "crypto.hpp"
#include "../../../defines.hpp"
#include "../../../ansi_escape.hpp"
#include "../../../logging/logging.hpp"
#include "../../../base64/base64.h"
#include <vector>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
namespace network::udp::crypto
{
    #define AES_IV_SIZE 8
    template <typename A, typename D>
    std::unique_ptr<A,D> make_handle(A* ptr, D deleter)
    {
        return std::unique_ptr<A,D>{ptr,deleter};
    }
    std::string pubkey_to_string(EVP_PKEY* x)
    {
        constexpr size_t BUFFER_LEN = 2048;
        std::unique_ptr<char> key_buffer{new char[BUFFER_LEN]};  
        memset(key_buffer.get(),'\0',BUFFER_LEN);
        auto bio = make_handle(BIO_new(BIO_s_mem()),BIO_free);
        PEM_write_bio_PUBKEY(bio.get(),x);
        BIO_read(bio.get(),key_buffer.get(),BUFFER_LEN-1);
        std::string ret = key_buffer.get();
        ret.erase(ret.begin(),ret.begin()+ret.find('\n'));
        ret.erase(ret.begin()+ret.find_last_of('\n'),ret.end());
        ret.erase(ret.begin()+ret.find_last_of('\n'),ret.end());
        ret.erase(std::remove(ret.begin(),ret.end(),'\n'),ret.cend());
        return ret;
    }
    std::unique_ptr<EVP_PKEY,decltype(&::EVP_PKEY_free)> string_to_pubkey(const std::string& s)
    {
        auto pem_content = "-----BEGIN PUBLIC KEY-----\n" + s + "\n-----END PUBLIC KEY-----\n";
        auto bio = make_handle(BIO_new_mem_buf(pem_content.c_str(),-1),BIO_free);
        EVP_PKEY* tmp_pkey = nullptr;
        if(not PEM_read_bio_PUBKEY(bio.get(),&tmp_pkey,nullptr,nullptr))
            throw std::runtime_error{"Invalid pubkey format"};
        return {tmp_pkey,EVP_PKEY_free};
    }
    std::string privkey_to_string(EVP_PKEY* x)
    {
        constexpr size_t BUFFER_LEN = 2048;
        std::unique_ptr<char> key_buffer{new char[BUFFER_LEN]};  
        memset(key_buffer.get(),'\0',BUFFER_LEN);
        auto bio = make_handle(BIO_new(BIO_s_mem()),BIO_free);
        PEM_write_bio_PrivateKey(bio.get(),x,nullptr,nullptr,0,nullptr,nullptr);
        BIO_read(bio.get(),key_buffer.get(),BUFFER_LEN-1);
        std::string ret = key_buffer.get();
        ret.erase(ret.begin(),ret.begin()+ret.find('\n'));
        ret.erase(ret.begin()+ret.find_last_of('\n'),ret.end());
        ret.erase(ret.begin()+ret.find_last_of('\n'),ret.end());
        ret.erase(std::remove(ret.begin(),ret.end(),'\n'),ret.cend());
        return ret;
    }
    std::unique_ptr<EVP_PKEY,decltype(&::EVP_PKEY_free)> string_to_privkey(const std::string& s)
    {
        auto pem_content = "-----BEGIN PRIVATE KEY-----\n" + s + "\n-----END PRIVATE KEY-----\n";
        auto bio = make_handle(BIO_new_mem_buf(pem_content.c_str(),-1),BIO_free);
        EVP_PKEY* tmp_pkey = nullptr;
        if(not PEM_read_bio_PrivateKey(bio.get(),&tmp_pkey,nullptr,nullptr))
            throw std::runtime_error{"Invalid pubkey format"};
        return {tmp_pkey,EVP_PKEY_free};
    }
    Key ecdhe(EVP_PKEY* local_private, EVP_PKEY* remote_public)
    {
        auto context = make_handle(EVP_PKEY_CTX_new(local_private,nullptr),EVP_PKEY_CTX_free);
        if(not EVP_PKEY_derive_init(context.get()))
            throw std::runtime_error{"EVP_PKEY_derive_init"};
        if(not EVP_PKEY_derive_set_peer(context.get(),remote_public))
            throw std::runtime_error{"EVP_PKEY_derive_set_peer"};
        size_t secret_len = 0;
        if(not EVP_PKEY_derive(context.get(),nullptr,&secret_len))
            throw std::runtime_error{"EVP_PKEY_derive"};
        std::unique_ptr<unsigned char> secret{new unsigned char[secret_len]};
        if(not EVP_PKEY_derive(context.get(),secret.get(),&secret_len))
            throw std::runtime_error{"EVP_PKEY_derive"};
        Key ret;
        SHA256(secret.get(),secret_len,ret.data);
        return ret;
    }
    std::unique_ptr<EVP_PKEY,decltype(&::EVP_PKEY_free)> gen_ecdhe_key()
    {
        try
        {
            auto context = make_handle(EVP_PKEY_CTX_new_id(EVP_PKEY_EC,nullptr),EVP_PKEY_CTX_free);
            if(not context)
                throw std::runtime_error{"Error creating a new context id"};
            if(not EVP_PKEY_keygen_init(context.get()))
                throw std::runtime_error{"Error initializing the keygen"};
            if(not EVP_PKEY_CTX_set_ec_paramgen_curve_nid(context.get(),NID_secp521r1))
                throw std::runtime_error{"Error selecting the secp521r1 EC"};
            EVP_PKEY* tmp_pkey = nullptr;
            if(not EVP_PKEY_keygen(context.get(),&tmp_pkey))
                throw std::runtime_error{"Error generating a key"};
            auto ret = make_handle(tmp_pkey,EVP_PKEY_free);
            return ret;
        }
        catch(const std::runtime_error& e)
        {
            logging::log("ERR","Something went wrong during the key generation: "+std::string(e.what()));
            return {nullptr,nullptr};
        }
    }
    std::tuple<std::string,std::string,std::string> encrypt(const std::string& message, const Key& key)
    {
        auto context = make_handle(EVP_CIPHER_CTX_new(),EVP_CIPHER_CTX_free);
        unsigned char iv[AES_IV_SIZE];
        unsigned char tag[AES_BLOCK_SIZE];
        auto ret = RAND_bytes(iv,AES_IV_SIZE);
        ret = EVP_EncryptInit_ex(context.get(),EVP_aes_256_gcm(),nullptr,nullptr,nullptr);
        ret = EVP_CIPHER_CTX_ctrl(context.get(),EVP_CTRL_GCM_SET_IVLEN,AES_IV_SIZE,nullptr);
        ret = EVP_CIPHER_CTX_set_key_length(context.get(),SHA256_DIGEST_LENGTH);
        ret = EVP_EncryptInit_ex(context.get(),nullptr,nullptr,key.data,iv);
        int total_len = 0;
        int outl = 0;
        std::vector<unsigned char> out(message.size()+AES_BLOCK_SIZE-message.size()%AES_BLOCK_SIZE);
        ret = EVP_EncryptUpdate(context.get(),out.data(),&outl,(unsigned char*)message.c_str(),(int)message.length());
        total_len += outl;
        ret = EVP_EncryptFinal_ex(context.get(),out.data()+outl,&outl);
        total_len += outl;
        ret = EVP_CIPHER_CTX_ctrl(context.get(),EVP_CTRL_GCM_GET_TAG,AES_BLOCK_SIZE,tag);

        std::unique_ptr<char> ret_data{new char[b64e_size((unsigned int)total_len)+1]};
        std::unique_ptr<char> ret_iv{new char[b64e_size(AES_IV_SIZE)+1]};
        std::unique_ptr<char> ret_tag{new char[b64e_size(AES_BLOCK_SIZE)+1]};
        b64_encode(out.data(),(unsigned int)total_len,(unsigned char*)ret_data.get());
        b64_encode(iv,AES_IV_SIZE,(unsigned char*)ret_iv.get());
        b64_encode(tag,AES_BLOCK_SIZE,(unsigned char*)ret_tag.get());
        return {ret_data.get(),ret_iv.get(),ret_tag.get()};
    }
    std::string decrypt(const std::string& cryptogram, const Key& key, const std::string& iv, const std::string& tag)
    {
        auto context = make_handle(EVP_CIPHER_CTX_new(),EVP_CIPHER_CTX_free);
        std::unique_ptr<unsigned char> raw_iv{new unsigned char[b64d_size((unsigned int)iv.length())]};
        std::unique_ptr<unsigned char> raw_tag{new unsigned char[b64d_size((unsigned int)tag.length())]};
        std::unique_ptr<unsigned char> raw_c{new unsigned char[b64d_size((unsigned int)cryptogram.length())]};
        auto iv_len = b64_decode((unsigned char*)iv.c_str(),(unsigned int)iv.length(),raw_iv.get());
        auto tag_len = b64_decode((unsigned char*)tag.c_str(),(unsigned int)tag.length(),raw_tag.get());
        if(iv_len != AES_IV_SIZE or tag_len != AES_BLOCK_SIZE)
            return "";
        auto c_len = b64_decode((unsigned char*)cryptogram.c_str(),(unsigned int)cryptogram.length(),raw_c.get());
        auto ret = EVP_DecryptInit_ex(context.get(),EVP_aes_256_gcm(),nullptr,nullptr,nullptr);
        ret = EVP_CIPHER_CTX_ctrl(context.get(),EVP_CTRL_GCM_SET_IVLEN,AES_IV_SIZE,nullptr);
        ret = EVP_CIPHER_CTX_set_key_length(context.get(),SHA256_DIGEST_LENGTH);
        ret = EVP_DecryptInit_ex(context.get(),nullptr,nullptr,key.data,raw_iv.get());
        int total_len = 0;
        int outl = 0;
        std::vector<unsigned char> out(c_len+1);
        ret = EVP_DecryptUpdate(context.get(),out.data(),&outl,raw_c.get(),c_len);
        total_len += outl;
        ret = EVP_CIPHER_CTX_ctrl(context.get(),EVP_CTRL_GCM_SET_TAG,AES_BLOCK_SIZE,raw_tag.get());
        ret = EVP_DecryptFinal_ex(context.get(),out.data()+outl,&outl);
        total_len += outl;
        out[total_len] = 0;
        return (char*)out.data();
    }
}