#pragma once

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <vector>
#include "cipher_interface.h"

namespace dpp::dave {

class openssl_aead_cipher : public cipher_interface {
public:
    openssl_aead_cipher(const EncryptionKey& encryptionKey);
    ~openssl_aead_cipher();

    bool IsValid() const { return cipherCtx_ != nullptr; }

    bool Encrypt(array_view<uint8_t> ciphertextBufferOut,
		 array_view<const uint8_t> plaintextBuffer,
		 array_view<const uint8_t> nonceBuffer,
		 array_view<const uint8_t> additionalData,
		 array_view<uint8_t> tagBufferOut) override;
    bool Decrypt(array_view<uint8_t> plaintextBufferOut,
		 array_view<const uint8_t> ciphertextBuffer,
		 array_view<const uint8_t> tagBuffer,
		 array_view<const uint8_t> nonceBuffer,
		 array_view<const uint8_t> additionalData) override;

private:
	EVP_CIPHER_CTX* cipherCtx_;  // Using EVP_CIPHER_CTX instead of EVP_AEAD_CTX
	std::vector<uint8_t> key_;

};

} // namespace dpp::dave

