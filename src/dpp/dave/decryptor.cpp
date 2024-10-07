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
#include "decryptor.h"
#include <bytes/bytes.h>
#include <dpp/cluster.h>
#include <cstring>
#include "common.h"
#include "leb128.h"
#include "scope_exit.h"

using namespace std::chrono_literals;

namespace dpp::dave {

constexpr auto kStatsInterval = 10s;

void decryptor::transition_to_key_ratchet(std::unique_ptr<key_ratchet_interface> keyRatchet,
					  duration transitionExpiry)
{
	if (keyRatchet) {
		creator.log(dpp::ll_trace, "Transitioning to new key ratchet, expiry: " + std::to_string(transitionExpiry.count()));
	}

	// Update the expiry time for all existing cryptor managers
	update_cryptor_manager_expiry(transitionExpiry);

	if (keyRatchet) {
		cryptorManagers_.emplace_back(creator, clock_, std::move(keyRatchet));
	}
}

void decryptor::transition_to_passthrough_mode(bool passthroughMode, duration transitionExpiry)
{
	if (passthroughMode) {
		allowPassThroughUntil_ = time_point::max();
	}
	else {
		// Update the pass through mode expiry
		auto maxExpiry = clock_.now() + transitionExpiry;
		allowPassThroughUntil_ = std::min(allowPassThroughUntil_, maxExpiry);
	}
}

size_t decryptor::decrypt(media_type mediaType,
			  array_view<const uint8_t> encryptedFrame,
			  array_view<uint8_t> frame)
{
	if (mediaType != media_audio && mediaType != media_video) {
		creator.log(dpp::ll_trace, "decrypt failed, invalid media type: " + std::to_string(static_cast<int>(mediaType)));
		return 0;
	}

	auto start = clock_.now();

	auto localFrame = get_or_create_frame_processor();
	scope_exit cleanup([&] { return_frame_processor(std::move(localFrame)); });

	// Skip decrypting for silence frames
	if (mediaType == media_audio && encryptedFrame.size() == OPUS_SILENCE_PACKET.size() &&
	std::memcmp(encryptedFrame.data(), OPUS_SILENCE_PACKET.data(), OPUS_SILENCE_PACKET.size()) == 0) {
		creator.log(dpp::ll_trace, "decrypt skipping silence of size: " + std::to_string(encryptedFrame.size()));
		if (encryptedFrame.data() != frame.data()) {
			std::memcpy(frame.data(), encryptedFrame.data(), encryptedFrame.size());
		}
		return encryptedFrame.size();
	}

	// Remove any expired cryptor manager
	cleanup_expired_cryptor_managers();

	// Process the incoming frame
	// This will check whether it looks like a valid encrypted frame
	// and if so it will parse it into its different components
	localFrame->parse_frame(encryptedFrame);

	// If the frame is not encrypted and we can pass it through, do it
	bool canUsePassThrough = allowPassThroughUntil_ > start;
	if (!localFrame->is_encrypted() && canUsePassThrough) {
		if (encryptedFrame.data() != frame.data()) {
		std::memcpy(frame.data(), encryptedFrame.data(), encryptedFrame.size());
		}
		stats_[mediaType].passthroughs++;
		return encryptedFrame.size();
	}

	// If the frame is not encrypted and we can't pass it through, fail
	if (!localFrame->is_encrypted()) {
		creator.log(dpp::ll_warning, "decrypt failed, frame is not encrypted and pass through is disabled");
		stats_[mediaType].decrypt_failure++;
		return 0;
	}

	// Try and decrypt with each valid cryptor
	// reverse iterate to try the newest cryptors first
	bool success = false;
	for (auto it = cryptorManagers_.rbegin(); it != cryptorManagers_.rend(); ++it) {
		auto& cryptorManager = *it;
		success = decrypt_impl(cryptorManager, mediaType, *localFrame, frame);
		if (success) {
			break;
		}
	}

	size_t bytesWritten = 0;
	if (success) {
		stats_[mediaType].decrypt_success++;
		bytesWritten = localFrame->reconstruct_frame(frame);
	}
	else {
		stats_[mediaType].decrypt_failure++;
		creator.log(dpp::ll_warning, "decrypt failed, no valid cryptor found, type: " + std::string(mediaType ? "video" : "audio") +
			", encrypted frame size: " + std::to_string(encryptedFrame.size()) +
			", plaintext frame size: " + std::to_string(frame.size()) +
			", number of cryptor managers: " + std::to_string(cryptorManagers_.size()) +
			", pass through enabled: " + std::string(canUsePassThrough ? "yes" : "no")
		);
	}

	auto end = clock_.now();
	stats_[mediaType].decrypt_duration += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

	return bytesWritten;
}

bool decryptor::decrypt_impl(aead_cipher_manager& cipher_manager,
				 media_type mediaType,
				 inbound_frame_processor& encryptedFrame,
				 array_view<uint8_t> frame)
{
	auto tag = encryptedFrame.get_tag();
	auto truncatedNonce = encryptedFrame.get_truncated_nonce();

	auto authenticatedData = encryptedFrame.get_authenticated_data();
	auto ciphertext = encryptedFrame.get_ciphertext();
	auto plaintext = encryptedFrame.get_plaintext();

	// expand the truncated nonce to the full sized one needed for decryption
	auto nonceBuffer = std::array<uint8_t, AES_GCM_128_NONCE_BYTES>();
	memcpy(nonceBuffer.data() + AES_GCM_128_TRUNCATED_SYNC_NONCE_OFFSET,
	   &truncatedNonce,
	   AES_GCM_128_TRUNCATED_SYNC_NONCE_BYTES);

	auto nonceBufferView = make_array_view<const uint8_t>(nonceBuffer.data(), nonceBuffer.size());

	auto generation =
		cipher_manager.compute_wrapped_generation(truncatedNonce >> RATCHET_GENERATION_SHIFT_BITS);

	if (!cipher_manager.can_process_nonce(generation, truncatedNonce)) {
		creator.log(dpp::ll_trace, "decrypt failed, cannot process nonce");
		return false;
	}

	// Get the cryptor for this generation
	cipher_interface* cipher = cipher_manager.get_cipher(generation);

	if (cipher == nullptr) {
		creator.log(dpp::ll_warning, "decrypt failed, no cryptor found for generation: " + std::to_string(generation));
		return false;
	}

	// perform the decryption
	bool success = cipher->decrypt(plaintext, ciphertext, tag, nonceBufferView, authenticatedData);
	stats_[mediaType].decrypt_attempts++;

	if (success) {
		cipher_manager.report_cipher_success(generation, truncatedNonce);
	}

	return success;
}

size_t decryptor::get_max_plaintext_byte_size(media_type mediaType, size_t encryptedFrameSize)
{
	return encryptedFrameSize;
}

void decryptor::update_cryptor_manager_expiry(duration expiry)
{
	auto maxExpiryTime = clock_.now() + expiry;
	for (auto& cryptorManager : cryptorManagers_) {
		cryptorManager.update_expiry(maxExpiryTime);
	}
}

void decryptor::cleanup_expired_cryptor_managers()
{
	while (!cryptorManagers_.empty() && cryptorManagers_.front().is_expired()) {
		creator.log(dpp::ll_trace, "Removing expired cryptor manager");
		cryptorManagers_.pop_front();
	}
}

std::unique_ptr<inbound_frame_processor> decryptor::get_or_create_frame_processor()
{
	std::lock_guard<std::mutex> lock(frameProcessorsMutex_);
	if (frameProcessors_.empty()) {
		return std::make_unique<inbound_frame_processor>(creator);
	}
	auto frameProcessor = std::move(frameProcessors_.back());
	frameProcessors_.pop_back();
	return frameProcessor;
}

void decryptor::return_frame_processor(std::unique_ptr<inbound_frame_processor> frameProcessor)
{
	std::lock_guard<std::mutex> lock(frameProcessorsMutex_);
	frameProcessors_.push_back(std::move(frameProcessor));
}

} // namespace dpp::dave

