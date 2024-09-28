#pragma once

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <vector>
#include "cipher_interface.h"

namespace dpp::dave {

class openssl_aead_cipher : public cipher_interface {
public:
	openssl_aead_cipher(const EncryptionKey& encryptionKey);

	/**
	 * Explicitly not copyable
	 */
	openssl_aead_cipher(openssl_aead_cipher&&) = delete;
	openssl_aead_cipher(openssl_aead_cipher&) = delete;
	openssl_aead_cipher operator=(openssl_aead_cipher&&) = delete;
	openssl_aead_cipher operator=(openssl_aead_cipher&) = delete;

	~openssl_aead_cipher() override;

	[[nodiscard]] bool inline is_valid() const {
		return cipherCtx_ != nullptr;
	}

	bool encrypt(byte_view ciphertextBufferOut, const_byte_view plaintextBuffer, const_byte_view nonceBuffer, const_byte_view additionalData, byte_view tagBufferOut) override;
	bool decrypt(byte_view plaintextBufferOut, const_byte_view ciphertextBuffer, const_byte_view tagBuffer, const_byte_view nonceBuffer, const_byte_view additionalData) override;

private:
	EVP_CIPHER_CTX* cipherCtx_;  // Using EVP_CIPHER_CTX instead of EVP_AEAD_CTX
	std::vector<uint8_t> key_;

};

} // namespace dpp::dave

