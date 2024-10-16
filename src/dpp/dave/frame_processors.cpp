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
#include "frame_processors.h"
#include <limits>
#include <optional>
#include <memory>
#include <cstring>
#include <dpp/exception.h>
#include <dpp/cluster.h>
#include "codec_utils.h"
#include "array_view.h"
#include "leb128.h"

#if defined(_MSC_VER)
	#include <intrin.h>
#endif

namespace dpp::dave {

#if defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
	/**
	 * @brief ARM does not have a builtin for overflow detecting add
	 * This implements a non-UB version of that.
	 * @param carry_in Input carry from previous add
	 * @param a First operand
	 * @param b Second operand
	 * @param result Output result
	 * @return True if overflow occured, false if it didn't
	 */
	inline uint8_t addcarry_size_t(size_t carry_in, size_t a, size_t b, size_t* result) {
		size_t partial_sum = a + b;
		uint8_t carry1 = (partial_sum < a);
		size_t final_sum = partial_sum + carry_in;
		uint8_t carry2 = (final_sum < partial_sum);
		*result = final_sum;
		return carry1 || carry2;
	}
#endif

std::pair<bool, size_t> overflow_add(size_t a, size_t b)
{
	size_t res;
#if defined(_MSC_VER) && defined(_M_X64)
	bool didOverflow = _addcarry_u64(0, a, b, &res);
#elif defined(_MSC_VER) && defined(_M_IX86)
	bool didOverflow = _addcarry_u32(0, a, b, &res);
#elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
	bool didOverflow = addcarry_size_t(0, a, b, &res);
#else
	bool didOverflow = __builtin_add_overflow(a, b, &res);
#endif
	return {didOverflow, res};
}

uint8_t unencrypted_ranges_size(const ranges& unencrypted_ranges)
{
	size_t size = 0;
	for (const auto& range : unencrypted_ranges) {
		size += leb128_size(range.offset);
		size += leb128_size(range.size);
	}
	return static_cast<uint8_t>(size);
}

uint8_t serialize_unencrypted_ranges(const ranges& unencrypted_ranges, uint8_t* buffer, size_t buffer_size)
{
	auto write_at = buffer;
	auto end = buffer + buffer_size;
	for (const auto& range : unencrypted_ranges) {
		auto range_size = leb128_size(range.offset) + leb128_size(range.size);
		if (range_size > static_cast<size_t>(end - write_at)) {
			break;
		}

		write_at += write_leb128(range.offset, write_at);
		write_at += write_leb128(range.size, write_at);
	}
	return static_cast<uint8_t>(write_at - buffer);
}

uint8_t deserialize_unencrypted_ranges(const uint8_t*& read_at, const uint8_t buffer_size, ranges& unencrypted_ranges)
{
	auto start = read_at;
	auto end = read_at + buffer_size;
	while (read_at < end) {
		size_t offset = read_leb128(read_at, end);
		if (read_at == nullptr) {
			break;
		}

		size_t size = read_leb128(read_at, end);
		if (read_at == nullptr) {
			break;
		}
		unencrypted_ranges.push_back({offset, size});
	}

	if (read_at != end) {
		unencrypted_ranges.clear();
		read_at = nullptr;
		return 0;
	}

	return static_cast<uint8_t>(read_at - start);
}

bool validate_unencrypted_ranges(const ranges& unencrypted_ranges, size_t frame_size)
{
	if (unencrypted_ranges.empty()) {
		return true;
	}

	// validate that the ranges are in order and don't overlap
	for (auto i = 0u; i < unencrypted_ranges.size(); ++i) {
		auto current = unencrypted_ranges[i];
		// The current range should not overflow into the next range
		// or if it is the last range, the end of the frame
		auto max_end =
		  i + 1 < unencrypted_ranges.size() ? unencrypted_ranges[i + 1].offset : frame_size;

		auto [did_overflow, current_end] = overflow_add(current.offset, current.size);
		if (did_overflow || current_end > max_end) {
			return false;
		}
	}

	return true;
}

size_t do_reconstruct(ranges ranges, const std::vector<uint8_t>& range_bytes, const std::vector<uint8_t>& other_bytes, const array_view<uint8_t>& output)
{
	size_t frame_index = 0;
	size_t range_bytes_index = 0;
	size_t other_bytes_index = 0;

	const auto copy_range_bytes = [&](size_t size) {
		std::memcpy(output.data() + frame_index, range_bytes.data() + range_bytes_index, size);
		range_bytes_index += size;
		frame_index += size;
	};

	const auto copy_other_bytes = [&](size_t size) {
		std::memcpy(output.data() + frame_index, other_bytes.data() + other_bytes_index, size);
		other_bytes_index += size;
		frame_index += size;
	};

	for (const auto& range : ranges) {
		if (range.offset > frame_index) {
			copy_other_bytes(range.offset - frame_index);
		}

		copy_range_bytes(range.size);
	}

	if (other_bytes_index < other_bytes.size()) {
		copy_other_bytes(other_bytes.size() - other_bytes_index);
	}

	return frame_index;
}

void inbound_frame_processor::clear()
{
	encrypted = false;
	original_size = 0;
	truncated_nonce = std::numeric_limits<truncated_sync_nonce>::max();
	unencrypted_ranges.clear();
	authenticated.clear();
	ciphertext.clear();
	plaintext.clear();
}

void inbound_frame_processor::parse_frame(array_view<const uint8_t> frame)
{
	clear();

	constexpr auto min_supplemental_bytes_size = AES_GCM_127_TRUNCATED_TAG_BYTES + sizeof(supplemental_bytes_size) + sizeof(magic_marker);
	if (frame.size() < min_supplemental_bytes_size) {
		creator.log(dpp::ll_warning, "Encrypted frame is too small to contain min supplemental bytes");
		return;
	}

	// Check the frame ends with the magic marker
	auto magic_marker_buffer = frame.end() - sizeof(magic_marker);
	if (memcmp(magic_marker_buffer, &MARKER_BYTES, sizeof(magic_marker)) != 0) {
		return;
	}

	// Read the supplemental bytes size
	supplemental_bytes_size bytes_size;
	auto bytes_size_buffer = magic_marker_buffer - sizeof(supplemental_bytes_size);
	memcpy(&bytes_size, bytes_size_buffer, sizeof(supplemental_bytes_size));

	// Check the frame is large enough to contain the supplemental bytes
	if (frame.size() < bytes_size) {
		creator.log(dpp::ll_warning, "Encrypted frame is too small to contain supplemental bytes");
		return;
	}

	// Check that supplemental bytes size is large enough to contain the supplemental bytes
	if (bytes_size < min_supplemental_bytes_size) {
		creator.log(dpp::ll_warning, "Supplemental bytes size is too small to contain supplemental bytes");
		return;
	}

	auto supplemental_bytes_buffer = frame.end() - bytes_size;

	// Read the tag
	tag = make_array_view(supplemental_bytes_buffer, AES_GCM_127_TRUNCATED_TAG_BYTES);

	// Read the nonce
	auto nonce_buffer = supplemental_bytes_buffer + AES_GCM_127_TRUNCATED_TAG_BYTES;
	auto read_at = nonce_buffer;
	auto end = bytes_size_buffer;
	truncated_nonce = static_cast<uint32_t>(read_leb128(read_at, end));
	if (read_at == nullptr) {
		creator.log(dpp::ll_warning, "Failed to read truncated nonce");
		return;
	}

	// Read the unencrypted ranges
	auto ranges_size = static_cast<uint8_t>(end - read_at);
	deserialize_unencrypted_ranges(read_at, ranges_size, unencrypted_ranges);
	if (read_at == nullptr) {
		creator.log(dpp::ll_warning, "Failed to read unencrypted ranges");
		return;
	}

	if (!validate_unencrypted_ranges(unencrypted_ranges, frame.size())) {
		creator.log(dpp::ll_warning, "Invalid unencrypted ranges");
		return;
	}

	// This is overly aggressive but will keep reallocations to a minimum
	authenticated.reserve(frame.size());
	ciphertext.reserve(frame.size());
	plaintext.reserve(frame.size());

	original_size = frame.size();

	// Split the frame into authenticated and ciphertext bytes
	size_t frame_index = 0;
	for (const auto& range : unencrypted_ranges) {
		auto encrypted_bytes = range.offset - frame_index;
		if (encrypted_bytes > 0) {
			add_ciphertext_bytes(frame.data() + frame_index, encrypted_bytes);
		}

		add_authenticated_bytes(frame.data() + range.offset, range.size);
		frame_index = range.offset + range.size;
	}
	auto actual_frame_size = frame.size() - bytes_size;
	if (frame_index < actual_frame_size) {
		add_ciphertext_bytes(frame.data() + frame_index, actual_frame_size - frame_index);
	}

	// Make sure the plaintext buffer is the same size as the ciphertext buffer
	plaintext.resize(ciphertext.size());

	// We've successfully parsed the frame
	// Mark the frame as encrypted
	encrypted = true;
}

size_t inbound_frame_processor::reconstruct_frame(array_view<uint8_t> frame) const
{
	if (!encrypted) {
		creator.log(dpp::ll_warning, "Cannot reconstruct an invalid encrypted frame");
		return 0;
	}

	if (authenticated.size() + plaintext.size() > frame.size()) {
		creator.log(dpp::ll_warning, "Frame is too small to contain the decrypted frame");
		return 0;
	}

	return do_reconstruct(unencrypted_ranges, authenticated, plaintext, frame);
}

void inbound_frame_processor::add_authenticated_bytes(const uint8_t* data, size_t size)
{
	authenticated.resize(authenticated.size() + size);
	memcpy(authenticated.data() + authenticated.size() - size, data, size);
}

void inbound_frame_processor::add_ciphertext_bytes(const uint8_t* data, size_t size)
{
	ciphertext.resize(ciphertext.size() + size);
	memcpy(ciphertext.data() + ciphertext.size() - size, data, size);
}

void outbound_frame_processor::reset()
{
	frame_codec = codec::cd_unknown;
	frame_index = 0;
	unencrypted_bytes.clear();
	encrypted_bytes.clear();
	unencrypted_ranges.clear();
}

void outbound_frame_processor::process_frame(array_view<const uint8_t> frame, codec codec)
{
	reset();

	frame_codec = codec;
	unencrypted_bytes.reserve(frame.size());
	encrypted_bytes.reserve(frame.size());

	bool success = false;
	switch (codec) {
	case codec::cd_opus:
		success = codec_utils::process_frame_opus(*this, frame);
		break;
	case codec::cd_vp8:
		success = codec_utils::process_frame_vp8(*this, frame);
		break;
	case codec::cd_vp9:
		success = codec_utils::process_frame_vp9(*this, frame);
		break;
	case codec::cd_h264:
		success = codec_utils::process_frame_h264(*this, frame);
		break;
	case codec::cd_h265:
		success = codec_utils::process_frame_h265(*this, frame);
		break;
	case codec::cd_av1:
		success = codec_utils::process_frame_av1(*this, frame);
		break;
	default:
		throw dpp::logic_exception("Unsupported codec for frame encryption");
	}

	if (!success) {
		frame_index = 0;
		unencrypted_bytes.clear();
		encrypted_bytes.clear();
		unencrypted_ranges.clear();
		add_encrypted_bytes(frame.data(), frame.size());
	}

	ciphertext_bytes.resize(encrypted_bytes.size());
}

size_t outbound_frame_processor::reconstruct_frame(array_view<uint8_t> frame)
{
	if (unencrypted_bytes.size() + ciphertext_bytes.size() > frame.size()) {
		creator.log(dpp::ll_warning, "Frame is too small to contain the encrypted frame");
		return 0;
	}

	return do_reconstruct(unencrypted_ranges, unencrypted_bytes, ciphertext_bytes, frame);
}

void outbound_frame_processor::add_unencrypted_bytes(const uint8_t* bytes, size_t size)
{
	if (!unencrypted_ranges.empty() &&
	    unencrypted_ranges.back().offset + unencrypted_ranges.back().size == frame_index) {
		// extend the last range
		unencrypted_ranges.back().size += size;
	} else {
		// add a new range (offset, size)
		unencrypted_ranges.push_back({frame_index, size});
	}

	unencrypted_bytes.resize(unencrypted_bytes.size() + size);
	memcpy(unencrypted_bytes.data() + unencrypted_bytes.size() - size, bytes, size);
	frame_index += size;
}

void outbound_frame_processor::add_encrypted_bytes(const uint8_t* bytes, size_t size)
{
	encrypted_bytes.resize(encrypted_bytes.size() + size);
	memcpy(encrypted_bytes.data() + encrypted_bytes.size() - size, bytes, size);
	frame_index += size;
}

}
