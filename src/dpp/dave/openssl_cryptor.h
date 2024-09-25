#pragma once

#include <openssl/evp.h>
#include "cryptor.h"

namespace discord {
namespace dave {

class BoringSSLCryptor : public ICryptor {
public:
    BoringSSLCryptor(const EncryptionKey& encryptionKey);
    ~BoringSSLCryptor();

    bool IsValid() const { /*return cipherCtx_.aead != nullptr;*/ return true; }

    bool Encrypt(ArrayView<uint8_t> ciphertextBufferOut,
                 ArrayView<const uint8_t> plaintextBuffer,
                 ArrayView<const uint8_t> nonceBuffer,
                 ArrayView<const uint8_t> additionalData,
                 ArrayView<uint8_t> tagBufferOut) override;
    bool Decrypt(ArrayView<uint8_t> plaintextBufferOut,
                 ArrayView<const uint8_t> ciphertextBuffer,
                 ArrayView<const uint8_t> tagBuffer,
                 ArrayView<const uint8_t> nonceBuffer,
                 ArrayView<const uint8_t> additionalData) override;

private:
    //EVP_AEAD_CTX cipherCtx_;
};

} // namespace dave
} // namespace discord
