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
#include "cryptor_manager.h"
#include <limits>
#include "key_ratchet.h"
#include <bytes/bytes.h>
#include <dpp/cluster.h>

using namespace std::chrono_literals;

namespace dpp::dave {

key_generation compute_wrapped_generation(key_generation oldest, key_generation generation)
{
	// Assume generation is greater than or equal to oldest, this may be wrong in a few cases but
	// will be caught by the max generation gap check.
	auto remainder = oldest % GENERATION_WRAP;
	auto factor = oldest / GENERATION_WRAP + (generation < remainder ? 1 : 0);
	return factor * GENERATION_WRAP + generation;
}

big_nonce compute_wrapped_big_nonce(key_generation generation, truncated_sync_nonce nonce)
{
	// Remove the generation bits from the nonce
	auto masked_nonce = nonce & ((1 << RATCHET_GENERATION_SHIFT_BITS) - 1);
	// Add the wrapped generation bits back in
	return static_cast<big_nonce>(generation) << RATCHET_GENERATION_SHIFT_BITS | masked_nonce;
}

aead_cipher_manager::aead_cipher_manager(dpp::cluster& cl, const clock_interface& clock, std::unique_ptr<key_ratchet_interface> key_ratchet)
 : current_clock(clock), current_key_ratchet(std::move(key_ratchet)), ratchet_creation(clock.now()), ratchet_expiry(time_point::max()), creator(cl) {
}

bool aead_cipher_manager::can_process_nonce(key_generation generation, truncated_sync_nonce nonce) const
{
	if (!newest_processed_nonce) {
		return true;
	}

	auto wrapped_big_nonce = compute_wrapped_big_nonce(generation, nonce);
	return wrapped_big_nonce > *newest_processed_nonce || std::find(missing_nonces.rbegin(), missing_nonces.rend(), wrapped_big_nonce) != missing_nonces.rend();
}

cipher_interface* aead_cipher_manager::get_cipher(key_generation generation)
{
	cleanup_expired_ciphers();

	if (generation < oldest_generation) {
		creator.log(dpp::ll_trace, "Received frame with old generation: " + std::to_string(generation) + ", oldest generation: " + std::to_string(oldest_generation));
		return nullptr;
	}

	if (generation > newest_generation + MAX_GENERATION_GAP) {
		creator.log(dpp::ll_trace, "Received frame with future generation: " + std::to_string(generation) + ", newest generation: " + std::to_string(newest_generation));
		return nullptr;
	}

	auto ratchet_lifetime_sec =
	  std::chrono::duration_cast<std::chrono::seconds>(current_clock.now() - ratchet_creation).count();
	auto max_lifetime_frames = MAX_FRAMES_PER_SECOND * ratchet_lifetime_sec;
	auto max_lifetime_generations = max_lifetime_frames >> RATCHET_GENERATION_SHIFT_BITS;
	if (generation > max_lifetime_generations) {
		creator.log(dpp::ll_debug, "Received frame with generation " + std::to_string(generation) + " beyond ratchet max lifetime generations: " + std::to_string(max_lifetime_generations) + ", ratchet lifetime: " + std::to_string(ratchet_lifetime_sec) + "s");
		return nullptr;
	}

	auto it = cryptor_generations.find(generation);
	if (it == cryptor_generations.end()) {
		// We don't have a cryptor for this generation, create one
		std::tie(it, std::ignore) = cryptor_generations.emplace(generation, make_expiring_cipher(generation));
	}

	// Return a non-owning pointer to the cryptor
	auto& [cryptor, expiry] = it->second;
	return cryptor.get();
}

void aead_cipher_manager::report_cipher_success(key_generation generation, truncated_sync_nonce nonce)
{
	auto wrapped_big_nonce = compute_wrapped_big_nonce(generation, nonce);

	// Add any missing nonces to the queue
	if (!newest_processed_nonce) {
		newest_processed_nonce = wrapped_big_nonce;
	}
	else if (wrapped_big_nonce > *newest_processed_nonce) {
		auto oldest_missing_nonce = wrapped_big_nonce > MAX_MISSING_NONCES ? wrapped_big_nonce - MAX_MISSING_NONCES : 0;

		while (!missing_nonces.empty() && missing_nonces.front() < oldest_missing_nonce) {
			missing_nonces.pop_front();
		}

		// If we're missing a lot, we don't want to add everything since newestProcessedNonce_
		auto missing_range_start = std::max(oldest_missing_nonce, *newest_processed_nonce + 1);
		for (auto i = missing_range_start; i < wrapped_big_nonce; ++i) {
			missing_nonces.push_back(i);
		}

		// Update the newest processed nonce
		newest_processed_nonce = wrapped_big_nonce;
	}
	else {
		auto it = std::find(missing_nonces.begin(), missing_nonces.end(), wrapped_big_nonce);
		if (it != missing_nonces.end()) {
			missing_nonces.erase(it);
		}
	}

	if (generation <= newest_generation || cryptor_generations.find(generation) == cryptor_generations.end()) {
		return;
	}
	creator.log(dpp::ll_trace, "Reporting cryptor success, generation: " + std::to_string(generation));
	newest_generation = generation;

	// Update the expiry time for all old cryptors
	const auto expiry_time = current_clock.now() + CIPHER_EXPIRY;
	for (auto& [gen, cryptor] : cryptor_generations) {
		if (gen < newest_generation) {
			creator.log(dpp::ll_trace, "Updating expiry for cryptor, generation: " + std::to_string(gen));
			cryptor.expiry = std::min(cryptor.expiry, expiry_time);
		}
	}
}

key_generation aead_cipher_manager::compute_wrapped_generation(key_generation generation)
{
	return ::dpp::dave::compute_wrapped_generation(oldest_generation, generation);
}

aead_cipher_manager::expiring_cipher aead_cipher_manager::make_expiring_cipher(key_generation generation)
{
	// Get the new key from the ratchet
	auto key = current_key_ratchet->get_key(generation);
	auto expiry_time = time_point::max();

	// If we got frames out of order, we might have to create a cryptor for an old generation
	// In that case, create it with a non-infinite expiry time as we have already transitioned
	// to a newer generation
	if (generation < newest_generation) {
		creator.log(dpp::ll_debug, "Creating cryptor for old generation: " + std::to_string(generation));
		expiry_time = current_clock.now() + CIPHER_EXPIRY;
	}
	else {
		creator.log(dpp::ll_debug, "Creating cryptor for new generation: " + std::to_string(generation));
	}

	return {create_cipher(creator, key), expiry_time};
}

void aead_cipher_manager::cleanup_expired_ciphers()
{
	for (auto it = cryptor_generations.begin(); it != cryptor_generations.end();) {
		auto& [generation, cryptor] = *it;

		bool expired = cryptor.expiry < current_clock.now();
		if (expired) {
			creator.log(dpp::ll_trace, "Removing expired cryptor, generation: " + std::to_string(generation));
		}

		it = expired ? cryptor_generations.erase(it) : ++it;
	}

	while (oldest_generation < newest_generation && cryptor_generations.find(oldest_generation) == cryptor_generations.end()) {
		creator.log(dpp::ll_trace, "Deleting key for old generation: " + std::to_string(oldest_generation));
		current_key_ratchet->delete_key(oldest_generation);
		++oldest_generation;
	}
}

}
