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

#include <filesystem>
#include <fstream>
#include <sstream>
#include <mutex>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <bytes/bytes.h>
#include <mls/crypto.h>
#include "parameters.h"
#include "version.h"

namespace dpp {
	class cluster;
}

namespace mlspp {
	struct SignaturePrivateKey;
};

namespace dpp::dave::mls {

/**
 * @brief Key pair context type
 */
using key_pair_context_type = const char *;

/**
 * @brief Get persisted key pair
 * @param ctx context (pass nullptr to generate transient key)
 * @param session_id session id (pass empty string to generate transient key)
 * @param version Protocol version
 * @return MLS signature private key
 */
std::shared_ptr<::mlspp::SignaturePrivateKey> get_persisted_key_pair(dpp::cluster& creator, key_pair_context_type ctx, const std::string& session_id, protocol_version version);

/**
 * @brief self signed signature and key
 */
struct KeyAndSelfSignature {
	/**
	 * @brief key
	 */
	std::vector<uint8_t> key;
	/**
	 * @brief signature
	 */
	std::vector<uint8_t> signature;
};

/**
 * @brief Get persisted public key
 * @param ctx context (set to nullptr to get transient key)
 * @param session_id session id (set to empty string to get transient key)
 * @param version protocol version
 * @return Key and self signature
 */
KeyAndSelfSignature get_persisted_public_key(dpp::cluster& creator, key_pair_context_type ctx, const std::string& session_id, signature_version version);

/**
 * @brief Delete persisted key pair
 * @param ctx context
 * @param session_id session ID
 * @param version protocol version
 * @return true if deleted
 */
bool delete_persisted_key_pair(dpp::cluster& creator, key_pair_context_type ctx, const std::string& session_id, signature_version version);

/**
 * @brief Key version for DAVE
 */
constexpr unsigned KeyVersion = 1;

namespace detail {
	/**
	 * Get generic persisted key pair
	 * @param ctx context
	 * @param id key ID
	 * @param suite ciphersuite
	 * @return signature and private key
	 */
	std::shared_ptr<::mlspp::SignaturePrivateKey> get_generic_persisted_key_pair(dpp::cluster& creator, key_pair_context_type ctx, const std::string& id, ::mlspp::CipherSuite suite);

	/**
	 * Delete generic persisted key pair
	 * @param ctx context
	 * @param id id
	 * @return true if deleted
	 */
	bool delete_generic_persisted_key_pair(dpp::cluster& creator, key_pair_context_type ctx, const std::string& id);
}

}
