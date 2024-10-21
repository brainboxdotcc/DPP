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
	 * @param key encryption key
	 */
	openssl_aead_cipher(dpp::cluster& _creator, const encryption_key& key);

	/**
	 * @brief Destructor
	 */
	~openssl_aead_cipher() override;

	/**
	 * @brief Returns true if valid
	 * @return True if valid
	 */
	[[nodiscard]] bool inline is_valid() const {
		return ssl_context != nullptr;
	}

	/**
	 * @brief Encrypt plaintext to ciphertext and authenticate it with tag/AAD
	 * @param ciphertext_buffer_out ciphertext
	 * @param plaintext_buffer plaintext
	 * @param nonce_buffer nonce/IV
	 * @param additional_data additional authenticated data
	 * @param tag_buffer_out tag
	 * @return True if encryption succeeded
	 */
	bool encrypt(byte_view ciphertext_buffer_out, const_byte_view plaintext_buffer, const_byte_view nonce_buffer, const_byte_view additional_data, byte_view tag_buffer_out) override;

	/**
	 * @brief Decrypt ciphertext to plaintext if it authenticates with tag/AAD
	 * @param plaintext_buffer_out plaintext
	 * @param ciphertext_buffer ciphertext
	 * @param tag_buffer tag
	 * @param nonce_buffer nonce/IV
	 * @param additional_data additional authenticated data
	 * @return True if decryption succeeded
	 */
	bool decrypt(byte_view plaintext_buffer_out, const_byte_view ciphertext_buffer, const_byte_view tag_buffer, const_byte_view nonce_buffer, const_byte_view additional_data) override;

private:
	/**
	 * @brief Using EVP_CIPHER_CTX instead of EVP_AEAD_CTX
	 */
	EVP_CIPHER_CTX* ssl_context;

	/**
	 * @brief Encryption/decryption key
	 */
	std::vector<uint8_t> aes_key;
};

} // namespace dpp::dave

