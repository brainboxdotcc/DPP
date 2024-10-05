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

#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>

#include "cipher_interface.h"
#include "key_ratchet.h"
#include "common.h"
#include "clock.h"

namespace dpp::dave {

key_generation compute_wrapped_generation(key_generation oldest, key_generation generation);

using big_nonce = uint64_t;
big_nonce compute_wrapped_big_nonce(key_generation generation, truncated_sync_nonce nonce);

class aead_cipher_manager {
public:
	using time_point = typename clock_interface::time_point;

	aead_cipher_manager(const clock_interface& clock, std::unique_ptr<key_ratchet_interface> keyRatchet);

	void update_expiry(time_point expiry) { ratchetExpiry_ = expiry; }
	bool is_expired() const { return clock_.now() > ratchetExpiry_; }

	bool can_process_nonce(key_generation generation, truncated_sync_nonce nonce) const;
	key_generation compute_wrapped_generation(key_generation generation);

	cipher_interface* get_cipher(key_generation generation);
	void report_cipher_success(key_generation generation, truncated_sync_nonce nonce);

private:
	struct expiring_cipher {
		std::unique_ptr<cipher_interface> cryptor;
		time_point expiry;
	};

	expiring_cipher make_expiring_cipher(key_generation generation);
	void cleanup_expired_ciphers();

	const clock_interface& clock_;
	std::unique_ptr<key_ratchet_interface> keyRatchet_;
	std::unordered_map<key_generation, expiring_cipher> cryptors_;

	time_point ratchetCreation_;
	time_point ratchetExpiry_;
	key_generation oldestGeneration_{0};
	key_generation newestGeneration_{0};

	std::optional<big_nonce> newestProcessedNonce_;
	std::deque<big_nonce> missingNonces_;
};

} // namespace dpp::dave

