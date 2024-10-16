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

void decryptor::transition_to_key_ratchet(std::unique_ptr<key_ratchet_interface> key_ratchet, duration transition_expiry)
{
	if (key_ratchet) {
		creator.log(dpp::ll_trace, "Transitioning to new key ratchet, expiry: " + std::to_string(transition_expiry.count()));
	}

	// Update the expiry time for all existing cryptor managers
	update_cryptor_manager_expiry(transition_expiry);

	if (key_ratchet) {
		cryptor_managers.emplace_back(creator, current_clock, std::move(key_ratchet));
	}
}

void decryptor::transition_to_passthrough_mode(bool passthrough_mode, duration transition_expiry)
{
	if (passthrough_mode) {
		allow_pass_through_until = time_point::max();
	}
	else {
		// Update the pass through mode expiry
		auto max_expiry = current_clock.now() + transition_expiry;
		allow_pass_through_until = std::min(allow_pass_through_until, max_expiry);
	}
}

size_t decryptor::decrypt(media_type this_media_type, array_view<const uint8_t> encrypted_frame, array_view<uint8_t> frame)
{
	if (this_media_type != media_audio && this_media_type != media_video) {
		creator.log(dpp::ll_trace, "decrypt failed, invalid media type: " + std::to_string(static_cast<int>(this_media_type)));
		return 0;
	}

	auto start = current_clock.now();

	auto local_frame = get_or_create_frame_processor();
	scope_exit cleanup([&] { return_frame_processor(std::move(local_frame)); });

	// Skip decrypting for silence frames
	if (this_media_type == media_audio && encrypted_frame.size() == OPUS_SILENCE_PACKET.size() && std::memcmp(encrypted_frame.data(), OPUS_SILENCE_PACKET.data(), OPUS_SILENCE_PACKET.size()) == 0) {
		creator.log(dpp::ll_trace, "decrypt skipping silence of size: " + std::to_string(encrypted_frame.size()));
		if (encrypted_frame.data() != frame.data()) {
			std::memcpy(frame.data(), encrypted_frame.data(), encrypted_frame.size());
		}
		return encrypted_frame.size();
	}

	// Remove any expired cryptor manager
	cleanup_expired_cryptor_managers();

	// Process the incoming frame
	// This will check whether it looks like a valid encrypted frame
	// and if so it will parse it into its different components
	local_frame->parse_frame(encrypted_frame);

	// If the frame is not encrypted and we can pass it through, do it
	bool can_use_pass_through = allow_pass_through_until > start;
	if (!local_frame->is_encrypted() && can_use_pass_through) {
		if (encrypted_frame.data() != frame.data()) {
		std::memcpy(frame.data(), encrypted_frame.data(), encrypted_frame.size());
		}
		stats[this_media_type].passthroughs++;
		return encrypted_frame.size();
	}

	// If the frame is not encrypted, and we can't pass it through, fail
	if (!local_frame->is_encrypted()) {
		creator.log(dpp::ll_warning, "decrypt failed, frame is not encrypted and pass through is disabled");
		stats[this_media_type].decrypt_failure++;
		return 0;
	}

	// Try and decrypt with each valid cryptor
	// reverse iterate to try the newest cryptors first
	bool success = false;
	for (auto it = cryptor_managers.rbegin(); it != cryptor_managers.rend(); ++it) {
		auto& cryptorManager = *it;
		success = decrypt_impl(cryptorManager, this_media_type, *local_frame, frame);
		if (success) {
			break;
		}
	}

	size_t bytes_written = 0;
	if (success) {
		stats[this_media_type].decrypt_success++;
		bytes_written = local_frame->reconstruct_frame(frame);
	}
	else {
		stats[this_media_type].decrypt_failure++;
		creator.log(dpp::ll_warning,
			"decrypt failed, no valid cryptor found, type: " + std::string(this_media_type ? "video" : "audio") +
			", encrypted frame size: " + std::to_string(encrypted_frame.size()) +
			", plaintext frame size: " + std::to_string(frame.size()) +
			", number of cryptor managers: " + std::to_string(cryptor_managers.size()) +
			", pass through enabled: " + std::string(can_use_pass_through ? "yes" : "no")
		);
	}

	auto end = current_clock.now();
	stats[this_media_type].decrypt_duration += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

	return bytes_written;
}

bool decryptor::decrypt_impl(aead_cipher_manager& cipher_manager, media_type this_media_type, inbound_frame_processor& encrypted_frame, array_view<uint8_t> frame)
{
	auto tag = encrypted_frame.get_tag();
	auto truncated_nonce = encrypted_frame.get_truncated_nonce();
	auto authenticated_data = encrypted_frame.get_authenticated_data();
	auto ciphertext_buffer = encrypted_frame.get_ciphertext();
	auto plaintext = encrypted_frame.get_plaintext();

	// expand the truncated nonce to the full sized one needed for decryption
	auto nonce_buffer = std::array<uint8_t, AES_GCM_128_NONCE_BYTES>();
	memcpy(nonce_buffer.data() + AES_GCM_128_TRUNCATED_SYNC_NONCE_OFFSET, &truncated_nonce, AES_GCM_128_TRUNCATED_SYNC_NONCE_BYTES);

	auto nonce_buffer_view = make_array_view<const uint8_t>(nonce_buffer.data(), nonce_buffer.size());

	auto generation = cipher_manager.compute_wrapped_generation(truncated_nonce >> RATCHET_GENERATION_SHIFT_BITS);

	if (!cipher_manager.can_process_nonce(generation, truncated_nonce)) {
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
	bool success = cipher->decrypt(plaintext, ciphertext_buffer, tag, nonce_buffer_view, authenticated_data);
	stats[this_media_type].decrypt_attempts++;

	if (success) {
		cipher_manager.report_cipher_success(generation, truncated_nonce);
	}

	return success;
}

size_t decryptor::get_max_plaintext_byte_size(media_type this_media_type, size_t encrypted_frame_size)
{
	return encrypted_frame_size;
}

void decryptor::update_cryptor_manager_expiry(duration expiry)
{
	auto max_expiry_time = current_clock.now() + expiry;
	for (auto& cryptorManager : cryptor_managers) {
		cryptorManager.update_expiry(max_expiry_time);
	}
}

void decryptor::cleanup_expired_cryptor_managers()
{
	while (!cryptor_managers.empty() && cryptor_managers.front().is_expired()) {
		creator.log(dpp::ll_trace, "Removing expired cryptor manager");
		cryptor_managers.pop_front();
	}
}

std::unique_ptr<inbound_frame_processor> decryptor::get_or_create_frame_processor()
{
	std::lock_guard<std::mutex> lock(frame_processors_mutex);
	if (frame_processors.empty()) {
		return std::make_unique<inbound_frame_processor>(creator);
	}
	auto frame_processor = std::move(frame_processors.back());
	frame_processors.pop_back();
	return frame_processor;
}

void decryptor::return_frame_processor(std::unique_ptr<inbound_frame_processor> frame_processor)
{
	std::lock_guard<std::mutex> lock(frame_processors_mutex);
	frame_processors.push_back(std::move(frame_processor));
}

}
