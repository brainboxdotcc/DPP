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
#include "encryptor.h"
#include <algorithm>
#include <cstring>
#include <bytes/bytes.h>
#include <dpp/exception.h>
#include <dpp/cluster.h>
#include "common.h"
#include "cryptor_manager.h"
#include "codec_utils.h"
#include "array_view.h"
#include "leb128.h"
#include "scope_exit.h"

using namespace std::chrono_literals;

namespace dpp::dave {

void encryptor::set_key_ratchet(std::unique_ptr<key_ratchet_interface> key_ratchet)
{
	std::lock_guard<std::mutex> lock(key_gen_mutex);
	ratchet = std::move(key_ratchet);
	cryptor = nullptr;
	current_key_generation = 0;
	truncated_nonce = 0;
}

void encryptor::set_passthrough_mode(bool passthrough_mode)
{
	passthrough_mode_enable = passthrough_mode;
	update_current_protocol_version(passthrough_mode ? 0 : max_protocol_version());
}

encryptor::result_code encryptor::encrypt(media_type this_media_type, uint32_t ssrc, array_view<const uint8_t> frame, array_view<uint8_t> encrypted_frame, size_t* bytes_written) {
	if (this_media_type != media_audio && this_media_type != media_video) {
		creator.log(dpp::ll_warning, "encrypt failed, invalid media type: " + std::to_string(static_cast<int>(this_media_type)));
		return result_code::rc_encryption_failure;
	}

	if (passthrough_mode_enable) {
		// Pass frame through without encrypting
		std::memcpy(encrypted_frame.data(), frame.data(), frame.size());
		*bytes_written = frame.size();
		stats[this_media_type].passthroughs++;
		return result_code::rc_success;
	}

	{
		std::lock_guard<std::mutex> lock(key_gen_mutex);
		if (!ratchet) {
			stats[this_media_type].encrypt_failure++;
			return result_code::rc_encryption_failure;
		}
	}

	auto start = std::chrono::steady_clock::now();
	auto result = result_code::rc_success;

	// write the codec identifier
	auto codec = codec_for_ssrc(ssrc);

	auto frame_processor = get_or_create_frame_processor();
	scope_exit cleanup([&] { return_frame_processor(std::move(frame_processor)); });

	frame_processor->process_frame(frame, codec);

	const auto& unencrypted_bytes = frame_processor->get_unencrypted_bytes();
	const auto& encrypted_bytes = frame_processor->get_encrypted_bytes();
	auto& ciphertext_bytes = frame_processor->get_ciphertext_bytes();

	const auto& unencrypted_ranges = frame_processor->get_unencrypted_ranges();
	auto ranges_size = unencrypted_ranges_size(unencrypted_ranges);

	auto additional_data = make_array_view(unencrypted_bytes.data(), unencrypted_bytes.size());
	auto plaintext_buffer = make_array_view(encrypted_bytes.data(), encrypted_bytes.size());
	auto ciphertext_buffer = make_array_view(ciphertext_bytes.data(), ciphertext_bytes.size());

	auto frame_size = encrypted_bytes.size() + unencrypted_bytes.size();
	auto tag_buffer = make_array_view(encrypted_frame.data() + frame_size, AES_GCM_127_TRUNCATED_TAG_BYTES);

	auto nonce_buffer = std::array<uint8_t, AES_GCM_128_NONCE_BYTES>();
	auto nonce_buffer_view = make_array_view<const uint8_t>(nonce_buffer.data(), nonce_buffer.size());

	constexpr auto MAX_CIPHERTEXT_VALIDATION_RETRIES = 10;

	// some codecs (e.g. H26X) have packetizers that cannot handle specific byte sequences
	// so we attempt up to MAX_CIPHERTEXT_VALIDATION_RETRIES to encrypt the frame
	// calling into codec utils to validate the ciphertext + supplemental section
	// and re-rolling the truncated nonce if it fails

	// the nonce increment will definitely change the ciphertext and the tag
	// incrementing the nonce will also change the appropriate bytes
	// in the tail end of the nonce
	// which can remove start codes from the last 1 or 2 bytes of the nonce
	// and the two bytes of the unencrypted header bytes
	for (auto attempt = 1; attempt <= MAX_CIPHERTEXT_VALIDATION_RETRIES; ++attempt) {
		auto [curr_cryptor, truncatedNonce] = get_next_cryptor_and_nonce();

		if (!curr_cryptor) {
			result = result_code::rc_encryption_failure;
			break;
		}

		// write the truncated nonce to our temporary full nonce array
		// (since the encryption call expects a full size nonce)
		std::memcpy(nonce_buffer.data() + AES_GCM_128_TRUNCATED_SYNC_NONCE_OFFSET, &truncatedNonce, AES_GCM_128_TRUNCATED_SYNC_NONCE_BYTES);

		// encrypt the plaintext, adding the unencrypted header to the tag
		bool success = curr_cryptor->encrypt(ciphertext_buffer, plaintext_buffer, nonce_buffer_view, additional_data, tag_buffer);

		stats[this_media_type].encrypt_attempts++;
		stats[this_media_type].encrypt_max_attempts =
		  std::max(stats[this_media_type].encrypt_max_attempts, (uint64_t)attempt);

		if (!success) {
			result = result_code::rc_encryption_failure;
			break;
		}

		auto reconstructed_frame_size = frame_processor->reconstruct_frame(encrypted_frame);

		auto size = leb128_size(truncatedNonce);

		auto truncated_nonce_buffer = make_array_view(tag_buffer.end(), size);
		auto unencrypted_ranges_buffer = make_array_view(truncated_nonce_buffer.end(), ranges_size);
		auto supplemental_bytes_buffer = make_array_view(unencrypted_ranges_buffer.end(), sizeof(supplemental_bytes_size));
		auto marker_bytes_buffer = make_array_view(supplemental_bytes_buffer.end(), sizeof(magic_marker));

		// write the nonce
		auto res = write_leb128(truncatedNonce, truncated_nonce_buffer.begin());
		if (res != size) {
			result = result_code::rc_encryption_failure;
			break;
		}

		// write the unencrypted ranges
		res = serialize_unencrypted_ranges(unencrypted_ranges, unencrypted_ranges_buffer.begin(), unencrypted_ranges_buffer.size());
		if (res != ranges_size) {
			result = result_code::rc_encryption_failure;
			break;
		}

		// write the supplemental bytes size
		uint64_t supplemental_bytes_large = SUPPLEMENTAL_BYTES + size + ranges_size;

		if (supplemental_bytes_large > std::numeric_limits<supplemental_bytes_size>::max()) {
			result = rc_encryption_failure;
			break;
		}

		supplemental_bytes_size supplemental_bytes = supplemental_bytes_large;
		std::memcpy(supplemental_bytes_buffer.data(), &supplemental_bytes, sizeof(supplemental_bytes_size));

		// write the marker bytes, ends the frame
		std::memcpy(marker_bytes_buffer.data(), &MARKER_BYTES, sizeof(magic_marker));

		auto encrypted_frame_bytes = reconstructed_frame_size + AES_GCM_127_TRUNCATED_TAG_BYTES + size + ranges_size + sizeof(supplemental_bytes_size) + sizeof(magic_marker);

		if (codec_utils::validate_encrypted_frame(*frame_processor, make_array_view(encrypted_frame.data(), encrypted_frame_bytes))) {
			*bytes_written = encrypted_frame_bytes;
			break;
		}
		else if (attempt >= MAX_CIPHERTEXT_VALIDATION_RETRIES) {
			result = result_code::rc_encryption_failure;
			break;
		}
	}

	auto now = std::chrono::steady_clock::now();
	stats[this_media_type].encrypt_duration += std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
	if (result == result_code::rc_success) {
		stats[this_media_type].encrypt_success++;
	}
	else {
		stats[this_media_type].encrypt_failure++;
	}

	return result;
}

size_t encryptor::get_max_ciphertext_byte_size(media_type this_media_type, size_t frame_size)
{
	return frame_size + SUPPLEMENTAL_BYTES + TRANSFORM_PADDING_BYTES;
}

void encryptor::assign_ssrc_to_codec(uint32_t ssrc, codec codec_type)
{
	auto existing_codec_it = std::find_if(
		ssrc_codec_pairs.begin(), ssrc_codec_pairs.end(), [ssrc](const ssrc_codec_pair& pair) {
			return pair.first == ssrc;
		}
	);

	if (existing_codec_it == ssrc_codec_pairs.end()) {
		ssrc_codec_pairs.emplace_back(ssrc, codec_type);
	}
	else {
		existing_codec_it->second = codec_type;
	}
}

codec encryptor::codec_for_ssrc(uint32_t ssrc)
{
	auto existing_codec_it = std::find_if(
		ssrc_codec_pairs.begin(), ssrc_codec_pairs.end(), [ssrc](const ssrc_codec_pair& pair) {
			return pair.first == ssrc;
		}
	);

	if (existing_codec_it != ssrc_codec_pairs.end()) {
		return existing_codec_it->second;
	}
	else {
		return codec::cd_opus;
	}
}

std::unique_ptr<outbound_frame_processor> encryptor::get_or_create_frame_processor()
{
	std::lock_guard<std::mutex> lock(frame_processors_mutex);
	if (frame_processors.empty()) {
		return std::make_unique<outbound_frame_processor>(creator);
	}
	auto frame_processor = std::move(frame_processors.back());
	frame_processors.pop_back();
	return frame_processor;
}

void encryptor::return_frame_processor(std::unique_ptr<outbound_frame_processor> frameProcessor)
{
	std::lock_guard<std::mutex> lock(frame_processors_mutex);
	frame_processors.push_back(std::move(frameProcessor));
}

encryptor::cryptor_and_nonce encryptor::get_next_cryptor_and_nonce()
{
	std::lock_guard<std::mutex> lock(key_gen_mutex);
	if (!ratchet) {
		return {nullptr, 0};
	}

	auto generation = compute_wrapped_generation(current_key_generation, ++truncated_nonce >> RATCHET_GENERATION_SHIFT_BITS);

	if (generation != current_key_generation || !cryptor) {
		current_key_generation = generation;

		auto key = ratchet->get_key(current_key_generation);
		cryptor = create_cipher(creator, key);
	}

	return {cryptor, truncated_nonce};
}

void encryptor::update_current_protocol_version(protocol_version version)
{
	if (version == current_protocol_version) {
		return;
	}

	current_protocol_version = version;
	if (changed_callback) {
		changed_callback();
	}
}

}
