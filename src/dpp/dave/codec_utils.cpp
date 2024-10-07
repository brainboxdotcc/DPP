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

unencrypted_frame_header_size BytesCoveringH264PPS(const uint8_t* payload, const uint64_t sizeRemaining)
{
	// the payload starts with three exponential golomb encoded values
	// (first_mb_in_slice, sps_id, pps_id)
	// the depacketizer needs the pps_id unencrypted
	// and the payload has RBSP encoding that we need to work around

	constexpr uint8_t kEmulationPreventionByte = 0x03;

	uint64_t payloadBitIndex = 0;
	auto zeroBitCount = 0;
	auto parsedExpGolombValues = 0;

	while (payloadBitIndex < sizeRemaining * 8 && parsedExpGolombValues < 3) {
		auto bitIndex = payloadBitIndex % 8;
		auto byteIndex = payloadBitIndex / 8;
		auto payloadByte = payload[byteIndex];

		// if we're starting a new byte
		// check if this is an emulation prevention byte
		// which we skip over
		if (bitIndex == 0) {
			if (byteIndex >= 2 && payloadByte == kEmulationPreventionByte &&
				payload[byteIndex - 1] == 0 && payload[byteIndex - 2] == 0) {
				payloadBitIndex += 8;
				continue;
			}
		}

		if ((payloadByte & (1 << (7 - bitIndex))) == 0) {
			// still in the run of leading zero bits
			++zeroBitCount;
			++payloadBitIndex;

			if (zeroBitCount >= 32) {
				throw dpp::length_exception("Unexpectedly large exponential golomb encoded value");
			}
		}
		else {
			// we hit a one
			// skip forward the number of bits dictated by the leading number of zeroes
			parsedExpGolombValues += 1;
			payloadBitIndex += 1 + zeroBitCount;
			zeroBitCount = 0;
		}
	}

	// return the number of bytes that covers the last exp golomb encoded value
	return (payloadBitIndex / 8) + 1;
}

const uint8_t kH26XNaluLongStartCode[] = {0, 0, 0, 1};
constexpr uint8_t kH26XNaluShortStartSequenceSize = 3;

using IndexStartCodeSizePair = std::pair<size_t, size_t>;

std::optional<IndexStartCodeSizePair> FindNextH26XNaluIndex(const uint8_t* buffer, const size_t bufferSize, const size_t searchStartIndex = 0)
{
	constexpr uint8_t kH26XStartCodeHighestPossibleValue = 1;
	constexpr uint8_t kH26XStartCodeEndByteValue = 1;
	constexpr uint8_t kH26XStartCodeLeadingBytesValue = 0;

	if (bufferSize < kH26XNaluShortStartSequenceSize) {
		return std::nullopt;
	}

	// look for NAL unit 3 or 4 byte start code
	for (size_t i = searchStartIndex; i < bufferSize - kH26XNaluShortStartSequenceSize;) {
		if (buffer[i + 2] > kH26XStartCodeHighestPossibleValue) {
			// third byte is not 0 or 1, can't be a start code
			i += kH26XNaluShortStartSequenceSize;
		}
		else if (buffer[i + 2] == kH26XStartCodeEndByteValue) {
			// third byte matches the start code end byte, might be a start code sequence
			if (buffer[i + 1] == kH26XStartCodeLeadingBytesValue &&
				buffer[i] == kH26XStartCodeLeadingBytesValue) {
				// confirmed start sequence {0, 0, 1}
				auto nalUnitStartIndex = i + kH26XNaluShortStartSequenceSize;

				if (i >= 1 && buffer[i - 1] == kH26XStartCodeLeadingBytesValue) {
					// 4 byte start code
					return std::optional<IndexStartCodeSizePair>({nalUnitStartIndex, 4});
				}
				else {
					// 3 byte start code
					return std::optional<IndexStartCodeSizePair>({nalUnitStartIndex, 3});
				}
			}

			i += kH26XNaluShortStartSequenceSize;
		}
		else {
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
	constexpr uint8_t kVP8KeyFrameUnencryptedBytes = 10;
	constexpr uint8_t kVP8DeltaFrameUnencryptedBytes = 1;

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

	size_t unencryptedHeaderBytes = 0;
	if ((frame.data()[0] & 0x01) == 0) {
		unencryptedHeaderBytes = kVP8KeyFrameUnencryptedBytes;
	}
	else {
		unencryptedHeaderBytes = kVP8DeltaFrameUnencryptedBytes;
	}

	processor.add_unencrypted_bytes(frame.data(), unencryptedHeaderBytes);
	processor.add_encrypted_bytes(frame.data() + unencryptedHeaderBytes,
					  frame.size() - unencryptedHeaderBytes);
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

	// constexpr uint8_t kH264SBit = 0x80;
	constexpr uint8_t kH264NalHeaderTypeMask = 0x1F;
	constexpr uint8_t kH264NalTypeSlice = 1;
	constexpr uint8_t kH264NalTypeIdr = 5;
	constexpr uint8_t kH264NalUnitHeaderSize = 1;

	// this frame can be packetized as a STAP-A or a FU-A
	// so we need to look at the first NAL units to determine how many bytes
	// the packetizer/depacketizer will need into the payload
	if (frame.size() < kH26XNaluShortStartSequenceSize + kH264NalUnitHeaderSize) {
		throw dpp::length_exception("H264 frame is too small to contain a NAL unit");
	}

	auto naluIndexPair = FindNextH26XNaluIndex(frame.data(), frame.size());
	while (naluIndexPair && naluIndexPair->first < frame.size() - 1) {
		auto [nalUnitStartIndex, startCodeSize] = *naluIndexPair;

		auto nalType = frame.data()[nalUnitStartIndex] & kH264NalHeaderTypeMask;

		// copy the start code and then the NAL unit

		// Because WebRTC will convert them all start codes to 4-byte on the receiver side
		// always write a long start code and then the NAL unit
		processor.add_unencrypted_bytes(kH26XNaluLongStartCode, sizeof(kH26XNaluLongStartCode));

		auto nextNaluIndexPair = FindNextH26XNaluIndex(frame.data(), frame.size(), nalUnitStartIndex);
		auto nextNaluStart = nextNaluIndexPair.has_value() ? nextNaluIndexPair->first - nextNaluIndexPair->second : frame.size();

		if (nalType == kH264NalTypeSlice || nalType == kH264NalTypeIdr) {
			// once we've hit a slice or an IDR
			// we just need to cover getting to the PPS ID
			auto nalUnitPayloadStart = nalUnitStartIndex + kH264NalUnitHeaderSize;
			auto nalUnitPPSBytes = BytesCoveringH264PPS(frame.data() + nalUnitPayloadStart, frame.size() - nalUnitPayloadStart);

		processor.add_unencrypted_bytes(frame.data() + nalUnitStartIndex, kH264NalUnitHeaderSize + nalUnitPPSBytes);
		processor.add_encrypted_bytes(
			frame.data() + nalUnitStartIndex + kH264NalUnitHeaderSize + nalUnitPPSBytes,
			nextNaluStart - nalUnitStartIndex - kH264NalUnitHeaderSize - nalUnitPPSBytes);
		}
		else {
			// copy the whole NAL unit
		processor.add_unencrypted_bytes(frame.data() + nalUnitStartIndex, nextNaluStart - nalUnitStartIndex);
		}

		naluIndexPair = nextNaluIndexPair;
	}

	return true;
}

bool process_frame_h265(outbound_frame_processor& processor, array_view<const uint8_t> frame)
{
	// minimize the amount of unencrypted header data for H265 depending on the NAL unit
	// type from WebRTC, see: src/modules/rtp_rtcp/source/rtp_format_h265.cc
	// src/common_video/h265/h265_common.cc
	// src/modules/rtp_rtcp/source/video_rtp_depacketizer_h265.cc

	constexpr uint8_t kH265NalHeaderTypeMask = 0x7E;
	constexpr uint8_t kH265NalTypeVclCutoff = 32;
	constexpr uint8_t kH265NalUnitHeaderSize = 2;

	// this frame can be packetized as a STAP-A or a FU-A
	// so we need to look at the first NAL units to determine how many bytes
	// the packetizer/depacketizer will need into the payload
	if (frame.size() < kH26XNaluShortStartSequenceSize + kH265NalUnitHeaderSize) {
		throw dpp::length_exception("H265 frame is too small to contain a NAL unit");
	}

	// look for NAL unit 3 or 4 byte start code
	auto naluIndexPair = FindNextH26XNaluIndex(frame.data(), frame.size());
	while (naluIndexPair && naluIndexPair->first < frame.size() - 1) {
		auto [nalUnitStartIndex, startCodeSize] = *naluIndexPair;

		uint8_t nalType = (frame.data()[nalUnitStartIndex] & kH265NalHeaderTypeMask) >> 1;

		// copy the start code and then the NAL unit

		// Because WebRTC will convert them all start codes to 4-byte on the receiver side
		// always write a long start code and then the NAL unit
		processor.add_unencrypted_bytes(kH26XNaluLongStartCode, sizeof(kH26XNaluLongStartCode));

		auto nextNaluIndexPair = FindNextH26XNaluIndex(frame.data(), frame.size(), nalUnitStartIndex);
		auto nextNaluStart = nextNaluIndexPair.has_value() ? nextNaluIndexPair->first - nextNaluIndexPair->second : frame.size();

		if (nalType < kH265NalTypeVclCutoff) {
			// found a VCL NAL, encrypt the payload only
			processor.add_unencrypted_bytes(frame.data() + nalUnitStartIndex, kH265NalUnitHeaderSize);
			processor.add_encrypted_bytes(frame.data() + nalUnitStartIndex + kH265NalUnitHeaderSize, nextNaluStart - nalUnitStartIndex - kH265NalUnitHeaderSize);
		} else {
			// copy the whole NAL unit
			processor.add_unencrypted_bytes(frame.data() + nalUnitStartIndex, nextNaluStart - nalUnitStartIndex);
		}

		naluIndexPair = nextNaluIndexPair;
	}

	return true;
}

bool process_frame_av1(outbound_frame_processor& processor, array_view<const uint8_t> frame)
{
	constexpr uint8_t kAv1ObuHeaderHasExtensionMask = 0b0'0000'100;
	constexpr uint8_t kAv1ObuHeaderHasSizeMask = 0b0'0000'010;
	constexpr uint8_t kAv1ObuHeaderTypeMask = 0b0'1111'000;
	constexpr uint8_t kObuTypeTemporalDelimiter = 2;
	constexpr uint8_t kObuTypeTileList = 8;
	constexpr uint8_t kObuTypePadding = 15;
	constexpr uint8_t kObuExtensionSizeBytes = 1;

	size_t i = 0;
	while (i < frame.size()) {
		// Read the OBU header.
		size_t obuHeaderIndex = i;
		uint8_t obuHeader = frame.data()[obuHeaderIndex];
		i += sizeof(obuHeader);

		bool obuHasExtension = obuHeader & kAv1ObuHeaderHasExtensionMask;
		bool obuHasSize = obuHeader & kAv1ObuHeaderHasSizeMask;
		int obuType = (obuHeader & kAv1ObuHeaderTypeMask) >> 3;

		if (obuHasExtension) {
			// Skip extension byte
			i += kObuExtensionSizeBytes;
		}

		if (i >= frame.size()) {
			// Malformed frame
			throw dpp::logic_exception("Malformed AV1 frame: header overflows frame");
		}

		size_t obuPayloadSize = 0;
		if (obuHasSize) {
			// Read payload size
			const uint8_t* start = frame.data() + i;
			const uint8_t* ptr = start;
			obuPayloadSize = read_leb128(ptr, frame.end());
			if (!ptr) {
				// Malformed frame
				throw dpp::logic_exception("Malformed AV1 frame: invalid LEB128 size");
			}
			i += ptr - start;
		}
		else {
			// If the size is not present, the OBU extends to the end of the frame.
			obuPayloadSize = frame.size() - i;
		}

		const auto obuPayloadIndex = i;

		if (i + obuPayloadSize > frame.size()) {
			// Malformed frame
			throw dpp::logic_exception("Malformed AV1 frame: payload overflows frame");
		}

		i += obuPayloadSize;

		// We only copy the OBUs that will not get dropped by the packetizer
		if (obuType != kObuTypeTemporalDelimiter && obuType != kObuTypeTileList &&
			obuType != kObuTypePadding) {
			// if this is the last OBU, we may need to flip the "has size" bit
			// which allows us to append necessary protocol data to the frame
			bool rewrittenWithoutSize = false;

			if (i == frame.size() && obuHasSize) {
				// Flip the "has size" bit
				obuHeader &= ~kAv1ObuHeaderHasSizeMask;
				rewrittenWithoutSize = true;
			}

			// write the OBU header unencrypted
		processor.add_unencrypted_bytes(&obuHeader, sizeof(obuHeader));
			if (obuHasExtension) {
				// write the extension byte unencrypted
				processor.add_unencrypted_bytes(frame.data() + obuHeaderIndex + sizeof(obuHeader), kObuExtensionSizeBytes);
			}

			// write the OBU payload size unencrypted if it was present and we didn't rewrite
			// without it
			if (obuHasSize && !rewrittenWithoutSize) {
				// The AMD AV1 encoder may pad LEB128 encoded sizes with a zero byte which the
				// webrtc packetizer removes. To prevent the packetizer from changing the frame,
				// we sanitize the size by re-writing it ourselves
				uint8_t leb128Buffer[LEB128_MAX_SIZE];
				size_t additionalBytesToWrite = write_leb128(obuPayloadSize, leb128Buffer);
				processor.add_unencrypted_bytes(leb128Buffer, additionalBytesToWrite);
			}

			// add the OBU payload, encrypted
			processor.add_encrypted_bytes(frame.data() + obuPayloadIndex, obuPayloadSize);
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

	constexpr size_t Padding = kH26XNaluShortStartSequenceSize - 1;

	const auto& unencryptedRanges = processor.get_unencrypted_ranges();

	// H264 and H265 ciphertexts cannot contain a 3 or 4 byte start code {0, 0, 1}
	// otherwise the packetizer gets confused
	// and the frame we get on the decryption side will be shifted and fail to decrypt
	size_t encryptedSectionStart = 0;
	for (auto& range : unencryptedRanges) {
		if (encryptedSectionStart == range.offset) {
			encryptedSectionStart += range.size;
			continue;
		}

		auto start = encryptedSectionStart - std::min(encryptedSectionStart, size_t{Padding});
		auto end = std::min(range.offset + Padding, frame.size());
		if (FindNextH26XNaluIndex(frame.data() + start, end - start)) {
			return false;
		}

		encryptedSectionStart = range.offset + range.size;
	}

	if (encryptedSectionStart == frame.size()) {
		return true;
	}

	auto start = encryptedSectionStart - std::min(encryptedSectionStart, size_t{Padding});
	auto end = frame.size();
	if (FindNextH26XNaluIndex(frame.data() + start, end - start)) {
		return false;
	}

	return true;
}

} // namespace dpp::dave::codec_utils


