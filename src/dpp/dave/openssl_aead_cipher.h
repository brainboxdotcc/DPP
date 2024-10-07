/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2021 Craig Edwards and D++ contributors
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/
#pragma once

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <vector>
#include "cipher_interface.h"

namespace dpp {
	class cluster;
}

namespace dpp::dave {

/**
 * @brief OpenSSL AES 128 GCM AEAD cipher
 *
 * Replaces the boringSSL AES cipher in the Discord implementation, so we don't
 * have a conflicting dependency.
 */
class openssl_aead_cipher : public cipher_interface { // NOLINT
public:

	/**
	 * @brief constructor
	 * @param _creator Creator
	 * @param encryptionKey encryption key
	 */
	openssl_aead_cipher(dpp::cluster& _creator, const encryption_key& encryptionKey);

	/**
	 * @brief Destructor
	 */
	~openssl_aead_cipher() override;

	/**
	 * @brief Returns true if valid
	 * @return True if valid
	 */
	[[nodiscard]] bool inline is_valid() const {
		return cipherCtx_ != nullptr;
	}

	/**
	 * @brief Encrypt plaintext to ciphertext and authenticate it with tag/AAD
	 * @param ciphertextBufferOut ciphertext
	 * @param plaintextBuffer plaintext
	 * @param nonceBuffer nonce/IV
	 * @param additionalData additional authenticated data
	 * @param tagBufferOut tag
	 * @return True if encryption succeeded
	 */
	bool encrypt(byte_view ciphertextBufferOut, const_byte_view plaintextBuffer, const_byte_view nonceBuffer, const_byte_view additionalData, byte_view tagBufferOut) override;

	/**
	 * @brief Decrypt ciphertext to plaintext if it authenticates with tag/AAD
	 * @param plaintextBufferOut plaintext
	 * @param ciphertextBuffer ciphertext
	 * @param tagBuffer tag
	 * @param nonceBuffer nonce/IV
	 * @param additionalData additional authenticated data
	 * @return True if decryption succeeded
	 */
	bool decrypt(byte_view plaintextBufferOut, const_byte_view ciphertextBuffer, const_byte_view tagBuffer, const_byte_view nonceBuffer, const_byte_view additionalData) override;

private:
	/**
	 * @brief Using EVP_CIPHER_CTX instead of EVP_AEAD_CTX
	 */
	EVP_CIPHER_CTX* cipherCtx_;

	/**
	 * @brief Encryption/decryption key
	 */
	std::vector<uint8_t> key_;
};

} // namespace dpp::dave

