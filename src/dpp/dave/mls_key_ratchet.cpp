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
#include "mls_key_ratchet.h"
#include <dpp/cluster.h>

namespace dpp::dave {

mls_key_ratchet::mls_key_ratchet(dpp::cluster& cl, ::mlspp::CipherSuite suite, bytes base_secret) noexcept : ratchet(suite, std::move(base_secret)), creator(cl) {
}

mls_key_ratchet::~mls_key_ratchet() noexcept = default;

encryption_key mls_key_ratchet::get_key(key_generation generation) noexcept
{
	creator.log(dpp::ll_debug, "Retrieving key for generation " + std::to_string(generation) + " from hash ratchet");
	try {
		auto key_and_nonce = ratchet.get(generation);
		return std::move(key_and_nonce.key.as_vec());
	}
	catch (const std::exception& e) {
		creator.log(dpp::ll_warning, "Failed to retrieve key for generation " + std::to_string(generation) + ": " + std::string(e.what()));
		return {};
	}
}

void mls_key_ratchet::delete_key(key_generation generation) noexcept
{
	ratchet.erase(generation);
}

}

