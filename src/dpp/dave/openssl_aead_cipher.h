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

namespace dpp::dave {

class openssl_aead_cipher : public cipher_interface { // NOLINT
public:
	openssl_aead_cipher(const EncryptionKey& encryptionKey);

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

