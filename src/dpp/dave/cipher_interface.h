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
 * This folder is a modified fork of libdave, https://github.com/discord/libdave
 * Copyright (c) 2024 Discord, Licensed under MIT
 *
 ************************************************************************************/
 #pragma once

#include <memory>

#include "common.h"
#include "array_view.h"

namespace dpp {
	class cluster;
}

namespace dpp::dave {

/**
 * @brief An array_view constant array of bytes
 */
using const_byte_view = array_view<const uint8_t>;

/**
 * @brief An array_view non-constant array of bytes
 */
using byte_view = array_view<uint8_t>;

/**
 * @brief Represents a block cipher with AEAD used to encrypt or decrypt
 * audio and video frames in the DAVE protocol.
 */
class cipher_interface { // NOLINT
public:
	/**
	 * @brief Create cipher interface
	 * @param _creator Creating cluster
	 */
	cipher_interface(dpp::cluster& _creator) : creator(_creator) { };

	/**
	 * @brief Default destructor
	 */
	virtual ~cipher_interface() = default;

	/**
	 * @brief Encrypt audio or video
	 * @param ciphertext_buffer_out Output buffer of ciphertext
	 * @param plaintext_buffer Input buffer for plaintext
	 * @param nonce_buffer Input nonce/IV
	 * @param additional_data Additional data for GCM AEAD encryption
	 * @param tag_buffer_out AEAD Tag for verification
	 * @return true if encryption succeeded, false if it failed
	 */
	virtual bool encrypt(byte_view ciphertext_buffer_out, const_byte_view plaintext_buffer, const_byte_view nonce_buffer, const_byte_view additional_data, byte_view tag_buffer_out) = 0;

	/**
	 * @brief Decrypt audio or video
	 * @param plaintext_buffer_out Output buffer for plaintext
	 * @param ciphertext_buffer Input buffer for ciphetext
	 * @param tag_buffer AEAD Tag for verification
	 * @param nonce_buffer Nonce/IV
	 * @param additional_data Additional data for GCM AEAD encryption
	 * @return true if decryption succeeded, false if it failed
	 */
	virtual bool decrypt(byte_view plaintext_buffer_out, const_byte_view ciphertext_buffer, const_byte_view tag_buffer, const_byte_view nonce_buffer, const_byte_view additional_data) = 0;

protected:

	/**
	 * @brief DPP Cluster, used for logging
	 */
	dpp::cluster& creator;
};

/**
 * @brief Factory function to create new cipher interface of the best supported type for DAVE
 * @param key encryption key
 * @return an instance of a class derived from cipher_interface
 */
std::unique_ptr<cipher_interface> create_cipher(dpp::cluster& cl, const encryption_key& key);

}
