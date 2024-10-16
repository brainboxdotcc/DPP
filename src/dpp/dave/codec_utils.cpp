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
#include "codec_utils.h"

#include <limits>
#include <optional>
#include <dpp/exception.h>
#include "leb128.h"

namespace dpp::dave::codec_utils {

unencrypted_frame_header_size bytes_covering_h264_pps(const uint8_t* payload, const uint64_t size_remaining) {
	// the payload starts with three exponential golomb encoded values
	// (first_mb_in_slice, sps_id, pps_id)
	// the depacketizer needs the pps_id unencrypted
	// and the payload has RBSP encoding that we need to work around

	constexpr uint8_t emulation_prevention_byte = 0x03;

	uint64_t payload_bit_index = 0;
	auto zero_bit_count = 0;
	auto parsed_exp_golomb_values = 0;

	while (payload_bit_index < size_remaining * 8 && parsed_exp_golomb_values < 3) {
		auto bit_index = payload_bit_index % 8;
		auto byte_index = payload_bit_index / 8;
		auto payload_byte = payload[byte_index];

		// if we're starting a new byte
		// check if this is an emulation prevention byte
		// which we skip over
		if (bit_index == 0) {
			if (byte_index >= 2 && payload_byte == emulation_prevention_byte && payload[byte_index - 1] == 0 && payload[byte_index - 2] == 0) {
				payload_bit_index += 8;
				continue;
			}
		}

		if ((payload_byte & (1 << (7 - bit_index))) == 0) {
			// still in the run of leading zero bits
			++zero_bit_count;
			++payload_bit_index;

			if (zero_bit_count >= 32) {
				throw dpp::length_exception("Unexpectedly large exponential golomb encoded value");
			}
		} else {
			// we hit a one
			// skip forward the number of bits dictated by the leading number of zeroes
			parsed_exp_golomb_values += 1;
			payload_bit_index += 1 + zero_bit_count;
			zero_bit_count = 0;
		}
	}

	// return the number of bytes that covers the last exp golomb encoded value
	auto result = (payload_bit_index / 8) + 1;
	if (result > std::numeric_limits<unencrypted_frame_header_size>::max()) {
		// bytes covering H264 PPS result cannot fit into unencrypted frame header size
		return 0;
	}
	return static_cast<unencrypted_frame_header_size>(result);
}

const uint8_t nalu_long_start_code[] = {0, 0, 0, 1};
constexpr uint8_t nalu_short_start_sequence_size = 3;

using index_start_code_size_pair = std::pair<size_t, size_t>;

std::optional<index_start_code_size_pair> next_h26x_nalu_index(const uint8_t* buffer, const size_t buffer_size, const size_t search_start_index = 0)
{
	constexpr uint8_t start_code_highest_possible_value = 1;
	constexpr uint8_t start_code_end_byte_value = 1;
	constexpr uint8_t start_code_leading_bytes_value = 0;

	if (buffer_size < nalu_short_start_sequence_size) {
		return std::nullopt;
	}

	// look for NAL unit 3 or 4 byte start code
	for (size_t i = search_start_index; i < buffer_size - nalu_short_start_sequence_size;) {
		if (buffer[i + 2] > start_code_highest_possible_value) {
			// third byte is not 0 or 1, can't be a start code
			i += nalu_short_start_sequence_size;
		} else if (buffer[i + 2] == start_code_end_byte_value) {
			// third byte matches the start code end byte, might be a start code sequence
			if (buffer[i + 1] == start_code_leading_bytes_value && buffer[i] == start_code_leading_bytes_value) {
				// confirmed start sequence {0, 0, 1}
				auto nal_unit_start_index = i + nalu_short_start_sequence_size;

				if (i >= 1 && buffer[i - 1] == start_code_leading_bytes_value) {
					// 4 byte start code
					return std::optional<index_start_code_size_pair>({nal_unit_start_index, 4});
				}
				else {
					// 3 byte start code
					return std::optional<index_start_code_size_pair>({nal_unit_start_index, 3});
				}
			}

			i += nalu_short_start_sequence_size;
		} else {
			// third byte is 0, might be a four byte start code
			++i;
		}
	}

	return std::nullopt;
}

bool process_frame_opus(outbound_frame_processor& processor, array_view<const uint8_t> frame)
{
	processor.add_encrypted_bytes(frame.data(), frame.size());
	return true;
}

bool process_frame_vp8(outbound_frame_processor& processor, array_view<const uint8_t> frame)
{
	constexpr uint8_t key_frame_unencrypted_bytes = 10;
	constexpr uint8_t delta_frame_unencrypted_bytes = 1;

	// parse the VP8 payload header to determine if it's a key frame
	// https://datatracker.ietf.org/doc/html/rfc7741#section-4.3

	// 0 1 2 3 4 5 6 7
	// +-+-+-+-+-+-+-+-+
	// |Size0|H| VER |P|
	// +-+-+-+-+-+-+-+-+
	// P is an inverse key frame flag

	// if this is a key frame the depacketizer will read 10 bytes into the payload header
	// if this is a delta frame the depacketizer only needs the first byte of the payload
	// header (since that's where the key frame flag is)

	size_t unencrypted_header_bytes = 0;
	if ((frame.data()[0] & 0x01) == 0) {
		unencrypted_header_bytes = key_frame_unencrypted_bytes;
	} else {
		unencrypted_header_bytes = delta_frame_unencrypted_bytes;
	}

	processor.add_unencrypted_bytes(frame.data(), unencrypted_header_bytes);
	processor.add_encrypted_bytes(frame.data() + unencrypted_header_bytes, frame.size() - unencrypted_header_bytes);
	return true;
}

bool process_frame_vp9(outbound_frame_processor& processor, array_view<const uint8_t> frame)
{
	// payload descriptor is unencrypted in each packet
	// and includes all information the depacketizer needs
	processor.add_encrypted_bytes(frame.data(), frame.size());
	return true;
}

bool process_frame_h264(outbound_frame_processor& processor, array_view<const uint8_t> frame)
{
	// minimize the amount of unencrypted header data for H264 depending on the NAL unit
	// type from WebRTC, see: src/modules/rtp_rtcp/source/rtp_format_h264.cc
	// src/common_video/h264/h264_common.cc
	// src/modules/rtp_rtcp/source/video_rtp_depacketizer_h264.cc

	constexpr uint8_t nal_header_type_mask = 0x1F;
	constexpr uint8_t nal_type_slice = 1;
	constexpr uint8_t nal_type_idr = 5;
	constexpr uint8_t nal_unit_header_size = 1;

	// this frame can be packetized as a STAP-A or a FU-A
	// so we need to look at the first NAL units to determine how many bytes
	// the packetizer/depacketizer will need into the payload
	if (frame.size() < nalu_short_start_sequence_size + nal_unit_header_size) {
		throw dpp::length_exception("H264 frame is too small to contain a NAL unit");
	}

	auto nalu_index_pair = next_h26x_nalu_index(frame.data(), frame.size());
	while (nalu_index_pair && nalu_index_pair->first < frame.size() - 1) {
		auto [nal_unit_start_index, start_code_size] = *nalu_index_pair;

		auto nal_type = frame.data()[nal_unit_start_index] & nal_header_type_mask;

		// copy the start code and then the NAL unit

		// Because WebRTC will convert them all start codes to 4-byte on the receiver side
		// always write a long start code and then the NAL unit
		processor.add_unencrypted_bytes(nalu_long_start_code, sizeof(nalu_long_start_code));

		auto next_nalu_index_pair = next_h26x_nalu_index(frame.data(), frame.size(), nal_unit_start_index);
		auto next_nalu_start = next_nalu_index_pair.has_value() ? next_nalu_index_pair->first - next_nalu_index_pair->second : frame.size();

		if (nal_type == nal_type_slice || nal_type == nal_type_idr) {
			// once we've hit a slice or an IDR
			// we just need to cover getting to the PPS ID
			auto nal_unit_payload_start = nal_unit_start_index + nal_unit_header_size;
			auto nal_unit_pps_bytes = bytes_covering_h264_pps(frame.data() + nal_unit_payload_start, frame.size() - nal_unit_payload_start);

		processor.add_unencrypted_bytes(frame.data() + nal_unit_start_index, nal_unit_header_size + nal_unit_pps_bytes);
		processor.add_encrypted_bytes(frame.data() + nal_unit_start_index + nal_unit_header_size + nal_unit_pps_bytes, next_nalu_start - nal_unit_start_index - nal_unit_header_size - nal_unit_pps_bytes);
		} else {
			// copy the whole NAL unit
			processor.add_unencrypted_bytes(frame.data() + nal_unit_start_index, next_nalu_start - nal_unit_start_index);
		}
		nalu_index_pair = next_nalu_index_pair;
	}

	return true;
}

bool process_frame_h265(outbound_frame_processor& processor, array_view<const uint8_t> frame)
{
	// minimize the amount of unencrypted header data for H265 depending on the NAL unit
	// type from WebRTC, see: src/modules/rtp_rtcp/source/rtp_format_h265.cc
	// src/common_video/h265/h265_common.cc
	// src/modules/rtp_rtcp/source/video_rtp_depacketizer_h265.cc

	constexpr uint8_t nal_header_type_mask = 0x7E;
	constexpr uint8_t nal_type_vcl_cutoff = 32;
	constexpr uint8_t nal_unit_header_size = 2;

	// this frame can be packetized as a STAP-A or a FU-A
	// so we need to look at the first NAL units to determine how many bytes
	// the packetizer/depacketizer will need into the payload
	if (frame.size() < nalu_short_start_sequence_size + nal_unit_header_size) {
		throw dpp::length_exception("H265 frame is too small to contain a NAL unit");
	}

	// look for NAL unit 3 or 4 byte start code
	auto nalu_index = next_h26x_nalu_index(frame.data(), frame.size());
	while (nalu_index && nalu_index->first < frame.size() - 1) {
		auto [nal_unit_start_index, start_code_size] = *nalu_index;

		uint8_t nal_type = (frame.data()[nal_unit_start_index] & nal_header_type_mask) >> 1;

		// copy the start code and then the NAL unit

		// Because WebRTC will convert them all start codes to 4-byte on the receiver side
		// always write a long start code and then the NAL unit
		processor.add_unencrypted_bytes(nalu_long_start_code, sizeof(nalu_long_start_code));

		auto next_nalu_index_pair = next_h26x_nalu_index(frame.data(), frame.size(), nal_unit_start_index);
		auto next_nalu_start = next_nalu_index_pair.has_value() ? next_nalu_index_pair->first - next_nalu_index_pair->second : frame.size();

		if (nal_type < nal_type_vcl_cutoff) {
			// found a VCL NAL, encrypt the payload only
			processor.add_unencrypted_bytes(frame.data() + nal_unit_start_index, nal_unit_header_size);
			processor.add_encrypted_bytes(frame.data() + nal_unit_start_index + nal_unit_header_size, next_nalu_start - nal_unit_start_index - nal_unit_header_size);
		} else {
			// copy the whole NAL unit
			processor.add_unencrypted_bytes(frame.data() + nal_unit_start_index, next_nalu_start - nal_unit_start_index);
		}

		nalu_index = next_nalu_index_pair;
	}

	return true;
}

bool process_frame_av1(outbound_frame_processor& processor, array_view<const uint8_t> frame)
{
	constexpr uint8_t obu_header_has_extension_mask = 0b0'0000'100;
	constexpr uint8_t obu_header_has_size_mask = 0b0'0000'010;
	constexpr uint8_t obu_header_type_mask = 0b0'1111'000;
	constexpr uint8_t obu_type_temporal_delimiter = 2;
	constexpr uint8_t obu_type_tile_list = 8;
	constexpr uint8_t obu_type_padding = 15;
	constexpr uint8_t obu_extension_size_bytes = 1;

	size_t i = 0;
	while (i < frame.size()) {
		// Read the OBU header.
		size_t obu_header_index = i;
		uint8_t obu_header = frame.data()[obu_header_index];
		i += sizeof(obu_header);

		bool obu_has_extension = obu_header & obu_header_has_extension_mask;
		bool obu_has_size = obu_header & obu_header_has_size_mask;
		int obu_type = (obu_header & obu_header_type_mask) >> 3;

		if (obu_has_extension) {
			// Skip extension byte
			i += obu_extension_size_bytes;
		}

		if (i >= frame.size()) {
			// Malformed frame
			throw dpp::logic_exception("Malformed AV1 frame: header overflows frame");
		}

		size_t obu_payload_size = 0;
		if (obu_has_size) {
			// Read payload size
			const uint8_t* start = frame.data() + i;
			const uint8_t* ptr = start;
			obu_payload_size = read_leb128(ptr, frame.end());
			if (!ptr) {
				// Malformed frame
				throw dpp::logic_exception("Malformed AV1 frame: invalid LEB128 size");
			}
			i += ptr - start;
		}
		else {
			// If the size is not present, the OBU extends to the end of the frame.
			obu_payload_size = frame.size() - i;
		}

		const auto obu_payload_index = i;

		if (i + obu_payload_size > frame.size()) {
			// Malformed frame
			throw dpp::logic_exception("Malformed AV1 frame: payload overflows frame");
		}

		i += obu_payload_size;

		// We only copy the OBUs that will not get dropped by the packetizer
		if (obu_type != obu_type_temporal_delimiter && obu_type != obu_type_tile_list && obu_type != obu_type_padding) {
			// if this is the last OBU, we may need to flip the "has size" bit
			// which allows us to append necessary protocol data to the frame
			bool rewritten_without_size = false;

			if (i == frame.size() && obu_has_size) {
				// Flip the "has size" bit
				obu_header &= ~obu_header_has_size_mask;
				rewritten_without_size = true;
			}

			// write the OBU header unencrypted
			processor.add_unencrypted_bytes(&obu_header, sizeof(obu_header));
			if (obu_has_extension) {
				// write the extension byte unencrypted
				processor.add_unencrypted_bytes(frame.data() + obu_header_index + sizeof(obu_header), obu_extension_size_bytes);
			}

			// write the OBU payload size unencrypted if it was present and we didn't rewrite
			// without it
			if (obu_has_size && !rewritten_without_size) {
				// The AMD AV1 encoder may pad LEB128 encoded sizes with a zero byte which the
				// webrtc packetizer removes. To prevent the packetizer from changing the frame,
				// we sanitize the size by re-writing it ourselves
				uint8_t leb128Buffer[LEB128_MAX_SIZE];
				size_t additionalBytesToWrite = write_leb128(obu_payload_size, leb128Buffer);
				processor.add_unencrypted_bytes(leb128Buffer, additionalBytesToWrite);
			}

			// add the OBU payload, encrypted
			processor.add_encrypted_bytes(frame.data() + obu_payload_index, obu_payload_size);
		}
	}

	return true;
}

bool validate_encrypted_frame(outbound_frame_processor& processor, array_view<uint8_t> frame)
{
	auto codec = processor.get_codec();
	if (codec != codec::cd_h264 && codec != codec::cd_h265) {
		return true;
	}

	constexpr size_t padding = nalu_short_start_sequence_size - 1;

	const auto& unencrypted_ranges = processor.get_unencrypted_ranges();

	// H264 and H265 ciphertexts cannot contain a 3 or 4 byte start code {0, 0, 1}
	// otherwise the packetizer gets confused
	// and the frame we get on the decryption side will be shifted and fail to decrypt
	size_t encrypted_section_start = 0;
	for (auto& range : unencrypted_ranges) {
		if (encrypted_section_start == range.offset) {
			encrypted_section_start += range.size;
			continue;
		}

		auto start = encrypted_section_start - std::min(encrypted_section_start, size_t{padding});
		auto end = std::min(range.offset + padding, frame.size());
		if (next_h26x_nalu_index(frame.data() + start, end - start)) {
			return false;
		}

		encrypted_section_start = range.offset + range.size;
	}

	if (encrypted_section_start == frame.size()) {
		return true;
	}

	auto start = encrypted_section_start - std::min(encrypted_section_start, size_t{padding});
	auto end = frame.size();
	if (next_h26x_nalu_index(frame.data() + start, end - start)) {
		return false;
	}

	return true;
}

}


