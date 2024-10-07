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
#include <iostream>
#include "common.h"

namespace dpp::dave {

openssl_aead_cipher::openssl_aead_cipher(dpp::cluster& _creator, const encryption_key& encryptionKey) :
	cipher_interface(_creator),
	cipherCtx_(EVP_CIPHER_CTX_new()),
	key_(std::vector(encryptionKey.data(), encryptionKey.data() + encryptionKey.size())) {
}

openssl_aead_cipher::~openssl_aead_cipher() {
	EVP_CIPHER_CTX_free(cipherCtx_);
}

bool openssl_aead_cipher::encrypt(byte_view ciphertextBufferOut, const_byte_view plaintextBuffer, const_byte_view nonceBuffer, const_byte_view additionalData, byte_view tagBufferOut) {
	
	int len{};

	if (EVP_EncryptInit_ex(cipherCtx_, EVP_aes_128_gcm(), nullptr, nullptr, nullptr) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Set IV length
	 */
	if (EVP_CIPHER_CTX_ctrl(cipherCtx_, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_128_NONCE_BYTES, nullptr) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/* Initialise key and IV */
	if (EVP_EncryptInit_ex(cipherCtx_, nullptr, nullptr, key_.data(), nonceBuffer.data()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Provide any AAD data. This can be called zero or more times as
	 * required
	 */
	if (EVP_EncryptUpdate(cipherCtx_, nullptr, &len, additionalData.data(), (int)additionalData.size()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if (EVP_EncryptUpdate(cipherCtx_, ciphertextBufferOut.data(), &len, plaintextBuffer.data(), (int)plaintextBuffer.size()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Finalise the encryption. Normally ciphertext bytes may be written at
	 * this stage, but this does not occur in GCM mode
	 */
	if (EVP_EncryptFinal_ex(cipherCtx_, ciphertextBufferOut.data() + len, &len) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/* Get the tag */
	if (EVP_CIPHER_CTX_ctrl(cipherCtx_, EVP_CTRL_GCM_GET_TAG, AES_GCM_127_TRUNCATED_TAG_BYTES, tagBufferOut.data()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	return true;
}

bool openssl_aead_cipher::decrypt(byte_view plaintextBufferOut, const_byte_view ciphertextBuffer, const_byte_view tagBuffer, const_byte_view nonceBuffer, const_byte_view additionalData) {

	int len = 0;

	/* Initialise the decryption operation. */
	if (EVP_DecryptInit_ex(cipherCtx_, EVP_aes_128_gcm(), nullptr, nullptr, nullptr) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/* Set IV length. Not necessary if this is 12 bytes (96 bits) */
	if (EVP_CIPHER_CTX_ctrl(cipherCtx_, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_128_NONCE_BYTES, nullptr) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;	
	}

	/* Initialise key and IV */
	if (EVP_DecryptInit_ex(cipherCtx_, nullptr, nullptr, key_.data(), nonceBuffer.data()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Provide any AAD data. This can be called zero or more times as
	 * required
	 */
	if (EVP_DecryptUpdate(cipherCtx_, nullptr, &len, additionalData.data(), (int)additionalData.size()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Provide the message to be decrypted, and obtain the plaintext output.
	 * EVP_DecryptUpdate can be called multiple times if necessary
	 */
	if (EVP_DecryptUpdate(cipherCtx_, plaintextBufferOut.data(), &len, ciphertextBuffer.data(), (int)ciphertextBuffer.size()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/* Set expected tag value. Works in OpenSSL 1.0.1d and later */
	if (EVP_CIPHER_CTX_ctrl(cipherCtx_, EVP_CTRL_GCM_SET_TAG, AES_GCM_127_TRUNCATED_TAG_BYTES, (void*)tagBuffer.data()) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	/*
	 * Finalise the decryption. A positive return value indicates success,
	 * anything else is a failure - the plaintext is not trustworthy.
	 */
	if (EVP_DecryptFinal_ex(cipherCtx_, plaintextBufferOut.data() + len, &len) == 0) {
		creator.log(dpp::ll_warning, "SSL Error: " + std::to_string(ERR_get_error()));
		return false;
	}

	return true;
}

} // namespace dpp::dave

