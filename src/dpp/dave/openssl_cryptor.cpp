#include "openssl_cryptor.h"

#include <openssl/err.h>

#include <bytes/bytes.h>

#include "common.h"
#include "logger.h"

namespace discord {
namespace dave {

void PrintSSLErrors()
{
    ERR_print_errors_cb(
      [](const char* str, size_t len, void* ctx) {
          DISCORD_LOG(LS_ERROR) << std::string(str, len);
          return 1;
      },
      nullptr);
}

OpenSSLCryptor::OpenSSLCryptor(const EncryptionKey& encryptionKey)
{
    /*EVP_AEAD_CTX_zero(&cipherCtx_);

    auto initResult = EVP_AEAD_CTX_init(&cipherCtx_,
                                        EVP_aead_aes_128_gcm(),
                                        encryptionKey.data(),
                                        encryptionKey.size(),
                                        kAesGcm128TruncatedTagBytes,
                                        nullptr);

    if (initResult != 1) {
        DISCORD_LOG(LS_ERROR) << "Failed to initialize AEAD context";
        PrintSSLErrors();
    }*/
}

OpenSSLCryptor::~OpenSSLCryptor()
{
    //EVP_AEAD_CTX_cleanup(&cipherCtx_);
}

bool OpenSSLCryptor::Encrypt(ArrayView<uint8_t> ciphertextBufferOut,
                               ArrayView<const uint8_t> plaintextBuffer,
                               ArrayView<const uint8_t> nonceBuffer,
                               ArrayView<const uint8_t> additionalData,
                               ArrayView<uint8_t> tagBufferOut)
{
    /*if (cipherCtx_.aead == nullptr) {
        DISCORD_LOG(LS_ERROR) << "Encrypt: AEAD context is not initialized";
        return false;
    }

    size_t tagSizeOut;
    auto encryptResult = EVP_AEAD_CTX_seal_scatter(&cipherCtx_,
                                                   ciphertextBufferOut.data(),
                                                   tagBufferOut.data(),
                                                   &tagSizeOut,
                                                   kAesGcm128TruncatedTagBytes,
                                                   nonceBuffer.data(),
                                                   kAesGcm128NonceBytes,
                                                   plaintextBuffer.data(),
                                                   plaintextBuffer.size(),
                                                   nullptr,
                                                   0,
                                                   additionalData.data(),
                                                   additionalData.size());
    if (encryptResult != 1) {
        DISCORD_LOG(LS_ERROR) << "Failed to encrypt data";
        PrintSSLErrors();
    }

    return encryptResult == 1;
     */
    return false;
}

bool OpenSSLCryptor::Decrypt(ArrayView<uint8_t> plaintextBufferOut,
                               ArrayView<const uint8_t> ciphertextBuffer,
                               ArrayView<const uint8_t> tagBuffer,
                               ArrayView<const uint8_t> nonceBuffer,
                               ArrayView<const uint8_t> additionalData)
{
    /*if (cipherCtx_.aead == nullptr) {
        DISCORD_LOG(LS_ERROR) << "Decrypt: AEAD context is not initialized";
        return false;
    }

    auto decryptResult = EVP_AEAD_CTX_open_gather(&cipherCtx_,
                                                  plaintextBufferOut.data(),
                                                  nonceBuffer.data(),
                                                  kAesGcm128NonceBytes,
                                                  ciphertextBuffer.data(),
                                                  ciphertextBuffer.size(),
                                                  tagBuffer.data(),
                                                  kAesGcm128TruncatedTagBytes,
                                                  additionalData.data(),
                                                  additionalData.size());

    return decryptResult == 1;
     */
    return false;
}

} // namespace dave
} // namespace discord
