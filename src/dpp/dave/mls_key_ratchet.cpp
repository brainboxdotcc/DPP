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
#include "logger.h"

namespace dpp::dave {

mls_key_ratchet::mls_key_ratchet(::mlspp::CipherSuite suite, bytes baseSecret) noexcept
  : hashRatchet_(suite, std::move(baseSecret))
{
}

mls_key_ratchet::~mls_key_ratchet() noexcept = default;

encryption_key mls_key_ratchet::get_key(key_generation generation) noexcept
{
	DISCORD_LOG(LS_INFO) << "Retrieving key for generation " << generation << " from HashRatchet";

	try {
		auto keyAndNonce = hashRatchet_.get(generation);
		return std::move(keyAndNonce.key.as_vec());
	}
	catch (const std::exception& e) {
		DISCORD_LOG(LS_ERROR) << "Failed to retrieve key for generation " << generation << ": "
							  << e.what();
		return {};
	}
}

void mls_key_ratchet::delete_key(key_generation generation) noexcept
{
	hashRatchet_.erase(generation);
}

} // namespace dpp::dave

