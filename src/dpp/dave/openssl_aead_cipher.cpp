#include "openssl_aead_cipher.h"
#include <openssl/err.h>
#include <openssl/evp.h>
#include <bytes/bytes.h>
#include "common.h"
#include "logger.h"

namespace dpp::dave {

void PrintSSLErrors()
{
    ERR_print_errors_cb(
      [](const char* str, size_t len, void* ctx) {
          DISCORD_LOG(LS_ERROR) << std::string(str, len);
          return 1;
      },
      nullptr);
}

openssl_aead_cipher::openssl_aead_cipher(const EncryptionKey& encryptionKey) {
	cipherCtx_ = EVP_CIPHER_CTX_new();
	key_ = std::vector(encryptionKey.data(), encryptionKey.data() + encryptionKey.size());
}

openssl_aead_cipher::~openssl_aead_cipher() {
	if (cipherCtx_) {
		EVP_CIPHER_CTX_free(cipherCtx_);
	}
}

bool openssl_aead_cipher::Encrypt(array_view<uint8_t> ciphertextBufferOut,
				  array_view<const uint8_t> plaintextBuffer,
				  array_view<const uint8_t> nonceBuffer,
				  array_view<const uint8_t> additionalData,
				  array_view<uint8_t> tagBufferOut) {
	const EVP_CIPHER* cipher = EVP_aes_256_gcm();

	if (EVP_EncryptInit_ex(cipherCtx_, cipher, nullptr, nullptr, nullptr) != 1) {
		return false;
	}
	if (EVP_CIPHER_CTX_ctrl(cipherCtx_, EVP_CTRL_AEAD_SET_IVLEN, nonceBuffer.size(), nullptr) != 1) {
		return false;
	}
	if (EVP_EncryptInit_ex(cipherCtx_, nullptr, nullptr, key_.data(), nonceBuffer.data()) != 1) {
		return false;
	}
	int len = 0;

	if (EVP_EncryptUpdate(cipherCtx_, nullptr, &len, additionalData.data(), additionalData.size()) != 1) { // AAD
		return false;
	}
	if (EVP_EncryptUpdate(cipherCtx_, ciphertextBufferOut.data(), &len, plaintextBuffer.data(), plaintextBuffer.size()) != 1) {
		return false;
	}
	if (EVP_EncryptFinal_ex(cipherCtx_, ciphertextBufferOut.data() + len, &len) != 1) {
		return false;
	}

	if (EVP_CIPHER_CTX_ctrl(cipherCtx_, EVP_CTRL_AEAD_GET_TAG, tagBufferOut.size(), tagBufferOut.data()) != 1) { // tag
		return false;
	}
	return true;
}

bool openssl_aead_cipher::Decrypt(array_view<uint8_t> plaintextBufferOut,
				  array_view<const uint8_t> ciphertextBuffer,
				  array_view<const uint8_t> tagBuffer,
				  array_view<const uint8_t> nonceBuffer,
				  array_view<const uint8_t> additionalData) {
	const EVP_CIPHER* cipher = EVP_aes_256_gcm();

	if (EVP_DecryptInit_ex(cipherCtx_, cipher, nullptr, nullptr, nullptr) != 1) {
		return false;
	}

	if (EVP_CIPHER_CTX_ctrl(cipherCtx_, EVP_CTRL_AEAD_SET_IVLEN, nonceBuffer.size(), nullptr) != 1) {
		return false;
	}
	if (EVP_DecryptInit_ex(cipherCtx_, nullptr, nullptr, key_.data(), nonceBuffer.data()) != 1) {
		return false;
	}

	int len = 0;

	if (EVP_DecryptUpdate(cipherCtx_, nullptr, &len, additionalData.data(), additionalData.size()) != 1) { // AAD
		return false;
	}
	if (EVP_DecryptUpdate(cipherCtx_, plaintextBufferOut.data(), &len, ciphertextBuffer.data(), ciphertextBuffer.size()) != 1) {
		return false;
	}
	if (EVP_CIPHER_CTX_ctrl(cipherCtx_, EVP_CTRL_AEAD_SET_TAG, tagBuffer.size(), (void*)tagBuffer.data()) != 1) {
		return false;
	}
	if (EVP_DecryptFinal_ex(cipherCtx_, plaintextBufferOut.data() + len, &len) <= 0) {
		return false;
	}
	return true;
}

} // namespace dpp::dave

