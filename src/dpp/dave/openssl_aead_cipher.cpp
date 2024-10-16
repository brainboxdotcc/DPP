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
#include "openssl_aead_cipher.h"
#include <openssl/err.h>
#include <openssl/evp.h>
#include <bytes/bytes.h>
#include <dpp/cluster.h>
#include "common.h"

namespace dpp::dave {

openssl_aead_cipher::openssl_aead_cipher(dpp::cluster& _creator, const encryption_key& key) :
	cipher_interface(_creator),
	ssl_context(EVP_CIPHER_CTX_new()),
	aes_key(std::vector(key.data(), key.data() + key.size())) {
}

openssl_aead_cipher::~openssl_aead_cipher() {
	EVP_CIPHER_CTX_free(ssl_context);
}

bool openssl_aead_cipher::encrypt(byte_view ciphertext_buffer_out, const_byte_view plaintext_buffer, const_byte_view nonce_buffer, const_byte_view additional_data, byte_view tag_buffer_out) {
	
	int len{};

	if (EVP_EncryptInit_ex(ssl_context, EVP_aes_128_gcm(), nullptr, nullptr, nullptr) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Set IV length
	 */
	if (EVP_CIPHER_CTX_ctrl(ssl_context, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_128_NONCE_BYTES, nullptr) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/* Initialise key and IV */
	if (EVP_EncryptInit_ex(ssl_context, nullptr, nullptr, aes_key.data(), nonce_buffer.data()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Provide any AAD data. This can be called zero or more times as
	 * required
	 */
	if (EVP_EncryptUpdate(ssl_context, nullptr, &len, additional_data.data(), (int)additional_data.size()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if (EVP_EncryptUpdate(ssl_context, ciphertext_buffer_out.data(), &len, plaintext_buffer.data(), (int)plaintext_buffer.size()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Finalise the encryption. Normally ciphertext bytes may be written at
	 * this stage, but this does not occur in GCM mode
	 */
	if (EVP_EncryptFinal_ex(ssl_context, ciphertext_buffer_out.data() + len, &len) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/* Get the tag */
	if (EVP_CIPHER_CTX_ctrl(ssl_context, EVP_CTRL_GCM_GET_TAG, AES_GCM_127_TRUNCATED_TAG_BYTES, tag_buffer_out.data()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	return true;
}

bool openssl_aead_cipher::decrypt(byte_view plaintext_buffer_out, const_byte_view ciphertext_buffer, const_byte_view tag_buffer, const_byte_view nonce_buffer, const_byte_view additional_data) {

	int len = 0;

	/* Initialise the decryption operation. */
	if (EVP_DecryptInit_ex(ssl_context, EVP_aes_128_gcm(), nullptr, nullptr, nullptr) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/* Set IV length. Not necessary if this is 12 bytes (96 bits) */
	if (EVP_CIPHER_CTX_ctrl(ssl_context, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_128_NONCE_BYTES, nullptr) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;	
	}

	/* Initialise key and IV */
	if (EVP_DecryptInit_ex(ssl_context, nullptr, nullptr, aes_key.data(), nonce_buffer.data()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Provide any AAD data. This can be called zero or more times as
	 * required
	 */
	if (EVP_DecryptUpdate(ssl_context, nullptr, &len, additional_data.data(), (int)additional_data.size()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Provide the message to be decrypted, and obtain the plaintext output.
	 * EVP_DecryptUpdate can be called multiple times if necessary
	 */
	if (EVP_DecryptUpdate(ssl_context, plaintext_buffer_out.data(), &len, ciphertext_buffer.data(), (int)ciphertext_buffer.size()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/* Set expected tag value. Works in OpenSSL 1.0.1d and later */
	if (EVP_CIPHER_CTX_ctrl(ssl_context, EVP_CTRL_GCM_SET_TAG, AES_GCM_127_TRUNCATED_TAG_BYTES, (void*)tag_buffer.data()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Finalise the decryption. A positive return value indicates success,
	 * anything else is a failure - the plaintext is not trustworthy.
	 */
	if (EVP_DecryptFinal_ex(ssl_context, plaintext_buffer_out.data() + len, &len) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	return true;
}

} // namespace dpp::dave

