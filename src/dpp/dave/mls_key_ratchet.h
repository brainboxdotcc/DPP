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

#include <mls/key_schedule.h>
#include "key_ratchet.h"

namespace dpp {
	class cluster;
}

namespace dpp::dave {

/**
 * @brief An implementation of the key ratchet using MLS
 */
class mls_key_ratchet : public key_ratchet_interface { // NOLINT
public:
	/**
	 * @brief Constructor
	 * @param suite MLS ciphersuite to use
	 * @param base_secret base secret
	 */
	mls_key_ratchet(dpp::cluster& cl, ::mlspp::CipherSuite suite, bytes base_secret) noexcept;

	/**
	 * @brief Destructor
	 */
	~mls_key_ratchet() noexcept override;

	/**
	 * @brief Gey key for ratchet
	 * @param generation current generation
	 * @return encryption key
	 */
	encryption_key get_key(key_generation generation) noexcept override;

	/**
	 * Delete key for ratchet
	 * @param generation current generation
	 */
	void delete_key(key_generation generation) noexcept override;

private:
	/**
	 * @brief MLS hash ratchet
	 */
	::mlspp::HashRatchet ratchet;

	/**
	 * @brief DPP Cluster, used for logging
	 */
	dpp::cluster& creator;
};

}

