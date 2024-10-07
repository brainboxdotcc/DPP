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
	auto maskedNonce = nonce & ((1 << RATCHET_GENERATION_SHIFT_BITS) - 1);
	// Add the wrapped generation bits back in
	return static_cast<big_nonce>(generation) << RATCHET_GENERATION_SHIFT_BITS | maskedNonce;
}

aead_cipher_manager::aead_cipher_manager(dpp::cluster& cl, const clock_interface& clock, std::unique_ptr<key_ratchet_interface> keyRatchet)
  : clock_(clock)
  , keyRatchet_(std::move(keyRatchet))
  , ratchetCreation_(clock.now())
  , ratchetExpiry_(time_point::max())
  , creator(cl)
{
}

bool aead_cipher_manager::can_process_nonce(key_generation generation, truncated_sync_nonce nonce) const
{
	if (!newestProcessedNonce_) {
		return true;
	}

	auto bigNonce = compute_wrapped_big_nonce(generation, nonce);
	return bigNonce > *newestProcessedNonce_ ||
	  std::find(missingNonces_.rbegin(), missingNonces_.rend(), bigNonce) != missingNonces_.rend();
}

cipher_interface* aead_cipher_manager::get_cipher(key_generation generation)
{
	cleanup_expired_ciphers();

	if (generation < oldestGeneration_) {
		creator.log(dpp::ll_trace, "Received frame with old generation: " + std::to_string(generation) + ", oldest generation: " + std::to_string(oldestGeneration_));
		return nullptr;
	}

	if (generation > newestGeneration_ + MAX_GENERATION_GAP) {
		creator.log(dpp::ll_trace, "Received frame with future generation: " + std::to_string(generation) + ", newest generation: " + std::to_string(newestGeneration_));
		return nullptr;
	}

	auto ratchetLifetimeSec =
	  std::chrono::duration_cast<std::chrono::seconds>(clock_.now() - ratchetCreation_).count();
	auto maxLifetimeFrames = MAX_FRAMES_PER_SECOND * ratchetLifetimeSec;
	auto maxLifetimeGenerations = maxLifetimeFrames >> RATCHET_GENERATION_SHIFT_BITS;
	if (generation > maxLifetimeGenerations) {
		creator.log(dpp::ll_debug, "Received frame with generation " + std::to_string(generation) + " beyond ratchet max lifetime generations: " + std::to_string(maxLifetimeGenerations) + ", ratchet lifetime: " + std::to_string(ratchetLifetimeSec) + "s");
		return nullptr;
	}

	auto it = cryptors_.find(generation);
	if (it == cryptors_.end()) {
		// We don't have a cryptor for this generation, create one
		std::tie(it, std::ignore) = cryptors_.emplace(generation, make_expiring_cipher(generation));
	}

	// Return a non-owning pointer to the cryptor
	auto& [cryptor, expiry] = it->second;
	return cryptor.get();
}

void aead_cipher_manager::report_cipher_success(key_generation generation, truncated_sync_nonce nonce)
{
	auto bigNonce = compute_wrapped_big_nonce(generation, nonce);

	// Add any missing nonces to the queue
	if (!newestProcessedNonce_) {
		newestProcessedNonce_ = bigNonce;
	}
	else if (bigNonce > *newestProcessedNonce_) {
		auto oldestMissingNonce = bigNonce > MAX_MISSING_NONCES ? bigNonce - MAX_MISSING_NONCES : 0;

		while (!missingNonces_.empty() && missingNonces_.front() < oldestMissingNonce) {
			missingNonces_.pop_front();
		}

		// If we're missing a lot, we don't want to add everything since newestProcessedNonce_
		auto missingRangeStart = std::max(oldestMissingNonce, *newestProcessedNonce_ + 1);
		for (auto i = missingRangeStart; i < bigNonce; ++i) {
			missingNonces_.push_back(i);
		}

		// Update the newest processed nonce
		newestProcessedNonce_ = bigNonce;
	}
	else {
		auto it = std::find(missingNonces_.begin(), missingNonces_.end(), bigNonce);
		if (it != missingNonces_.end()) {
			missingNonces_.erase(it);
		}
	}

	if (generation <= newestGeneration_ || cryptors_.find(generation) == cryptors_.end()) {
		return;
	}
	creator.log(dpp::ll_trace, "Reporting cryptor success, generation: " + std::to_string(generation));
	newestGeneration_ = generation;

	// Update the expiry time for all old cryptors
	const auto expiryTime = clock_.now() + CIPHER_EXPIRY;
	for (auto& [gen, cryptor] : cryptors_) {
		if (gen < newestGeneration_) {
			creator.log(dpp::ll_trace, "Updating expiry for cryptor, generation: " + std::to_string(gen));
			cryptor.expiry = std::min(cryptor.expiry, expiryTime);
		}
	}
}

key_generation aead_cipher_manager::compute_wrapped_generation(key_generation generation)
{
	return ::dpp::dave::compute_wrapped_generation(oldestGeneration_, generation);
}

aead_cipher_manager::expiring_cipher aead_cipher_manager::make_expiring_cipher(key_generation generation)
{
	// Get the new key from the ratchet
	auto encryptionKey = keyRatchet_->get_key(generation);
	auto expiryTime = time_point::max();

	// If we got frames out of order, we might have to create a cryptor for an old generation
	// In that case, create it with a non-infinite expiry time as we have already transitioned
	// to a newer generation
	if (generation < newestGeneration_) {
		creator.log(dpp::ll_debug, "Creating cryptor for old generation: " + std::to_string(generation));
		expiryTime = clock_.now() + CIPHER_EXPIRY;
	}
	else {
		creator.log(dpp::ll_debug, "Creating cryptor for new generation: " + std::to_string(generation));
	}

	return {create_cipher(creator, encryptionKey), expiryTime};
}

void aead_cipher_manager::cleanup_expired_ciphers()
{
	for (auto it = cryptors_.begin(); it != cryptors_.end();) {
		auto& [generation, cryptor] = *it;

		bool expired = cryptor.expiry < clock_.now();
		if (expired) {
			creator.log(dpp::ll_trace, "Removing expired cryptor, generation: " + std::to_string(generation));
		}

		it = expired ? cryptors_.erase(it) : ++it;
	}

	while (oldestGeneration_ < newestGeneration_ && cryptors_.find(oldestGeneration_) == cryptors_.end()) {
		creator.log(dpp::ll_trace, "Deleting key for old generation: " + std::to_string(oldestGeneration_));
		keyRatchet_->delete_key(oldestGeneration_);
		++oldestGeneration_;
	}
}

} // namespace dpp::dave

