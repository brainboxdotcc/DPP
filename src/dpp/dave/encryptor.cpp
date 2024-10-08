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

constexpr auto kStatsInterval = 10s;

void encryptor::set_key_ratchet(std::unique_ptr<key_ratchet_interface> keyRatchet)
{
	std::lock_guard<std::mutex> lock(keyGenMutex_);
	keyRatchet_ = std::move(keyRatchet);
	cryptor_ = nullptr;
	currentKeyGeneration_ = 0;
	truncatedNonce_ = 0;
}

void encryptor::set_passthrough_mode(bool passthroughMode)
{
	passthroughMode_ = passthroughMode;
	update_current_protocol_version(passthroughMode ? 0 : max_protocol_version());
}

encryptor::result_code encryptor::encrypt(media_type mediaType,
			   uint32_t ssrc,
			   array_view<const uint8_t> frame,
			   array_view<uint8_t> encryptedFrame,
			   size_t* bytesWritten)
{
	if (mediaType != media_audio && mediaType != media_video) {
		creator.log(dpp::ll_warning, "encrypt failed, invalid media type: " + std::to_string(static_cast<int>(mediaType)));
		return result_code::rc_encryption_failure;
	}

	if (passthroughMode_) {
		// Pass frame through without encrypting
	std::memcpy(encryptedFrame.data(), frame.data(), frame.size());
		*bytesWritten = frame.size();
		stats_[mediaType].passthroughs++;
		return result_code::rc_success;
	}

	{
		std::lock_guard<std::mutex> lock(keyGenMutex_);
		if (!keyRatchet_) {
			stats_[mediaType].encrypt_failure++;
			return result_code::rc_encryption_failure;
		}
	}

	auto start = std::chrono::steady_clock::now();
	auto result = result_code::rc_success;

	// write the codec identifier
	auto codec = codec_for_ssrc(ssrc);

	auto frameProcessor = get_or_create_frame_processor();
	scope_exit cleanup([&] { return_frame_processor(std::move(frameProcessor)); });

	frameProcessor->process_frame(frame, codec);

	const auto& unencryptedBytes = frameProcessor->get_unencrypted_bytes();
	const auto& encryptedBytes = frameProcessor->get_encrypted_bytes();
	auto& ciphertextBytes = frameProcessor->get_ciphertext_bytes();

	const auto& unencryptedRanges = frameProcessor->get_unencrypted_ranges();
	auto unencryptedRangesSize = unencrypted_ranges_size(unencryptedRanges);

	auto additionalData = make_array_view(unencryptedBytes.data(), unencryptedBytes.size());
	auto plaintextBuffer = make_array_view(encryptedBytes.data(), encryptedBytes.size());
	auto ciphertextBuffer = make_array_view(ciphertextBytes.data(), ciphertextBytes.size());

	auto frameSize = encryptedBytes.size() + unencryptedBytes.size();
	auto tagBuffer = make_array_view(encryptedFrame.data() + frameSize, AES_GCM_127_TRUNCATED_TAG_BYTES);

	auto nonceBuffer = std::array<uint8_t, AES_GCM_128_NONCE_BYTES>();
	auto nonceBufferView = make_array_view<const uint8_t>(nonceBuffer.data(), nonceBuffer.size());

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
		auto [cryptor, truncatedNonce] = get_next_cryptor_and_nonce();

		if (!cryptor) {
			result = result_code::rc_encryption_failure;
			break;
		}

		// write the truncated nonce to our temporary full nonce array
		// (since the encryption call expects a full size nonce)
		std::memcpy(nonceBuffer.data() + AES_GCM_128_TRUNCATED_SYNC_NONCE_OFFSET,
			&truncatedNonce,
			AES_GCM_128_TRUNCATED_SYNC_NONCE_BYTES);

		// encrypt the plaintext, adding the unencrypted header to the tag
		bool success = cryptor->encrypt(
		ciphertextBuffer, plaintextBuffer, nonceBufferView, additionalData, tagBuffer);

		stats_[mediaType].encrypt_attempts++;
		stats_[mediaType].encrypt_max_attempts =
		  std::max(stats_[mediaType].encrypt_max_attempts, (uint64_t)attempt);

		if (!success) {
			result = result_code::rc_encryption_failure;
			break;
		}

		auto reconstructedFrameSize = frameProcessor->reconstruct_frame(encryptedFrame);

		auto nonceSize = leb128_size(truncatedNonce);

		auto truncatedNonceBuffer = make_array_view(tagBuffer.end(), nonceSize);
		auto unencryptedRangesBuffer =
		make_array_view(truncatedNonceBuffer.end(), unencryptedRangesSize);
		auto supplementalBytesBuffer =
		make_array_view(unencryptedRangesBuffer.end(), sizeof(supplemental_bytes_size));
		auto markerBytesBuffer = make_array_view(supplementalBytesBuffer.end(), sizeof(magic_marker));

		// write the nonce
		auto res = write_leb128(truncatedNonce, truncatedNonceBuffer.begin());
		if (res != nonceSize) {
			result = result_code::rc_encryption_failure;
			break;
		}

		// write the unencrypted ranges
		res = serialize_unencrypted_ranges(
			unencryptedRanges, unencryptedRangesBuffer.begin(), unencryptedRangesBuffer.size());
		if (res != unencryptedRangesSize) {
			result = result_code::rc_encryption_failure;
			break;
		}

		// write the supplemental bytes size
		supplemental_bytes_size supplementalBytes =
		SUPPLEMENTAL_BYTES + nonceSize + unencryptedRangesSize;
	std::memcpy(supplementalBytesBuffer.data(), &supplementalBytes, sizeof(supplemental_bytes_size));

		// write the marker bytes, ends the frame
	std::memcpy(markerBytesBuffer.data(), &MARKER_BYTES, sizeof(magic_marker));

		auto encryptedFrameBytes = reconstructedFrameSize + AES_GCM_127_TRUNCATED_TAG_BYTES +
				   nonceSize + unencryptedRangesSize + sizeof(supplemental_bytes_size) + sizeof(magic_marker);

		if (codec_utils::validate_encrypted_frame(
		*frameProcessor, make_array_view(encryptedFrame.data(), encryptedFrameBytes))) {
			*bytesWritten = encryptedFrameBytes;
			break;
		}
		else if (attempt >= MAX_CIPHERTEXT_VALIDATION_RETRIES) {
			result = result_code::rc_encryption_failure;
			break;
		}
	}

	auto now = std::chrono::steady_clock::now();
	stats_[mediaType].encrypt_duration +=
	  std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
	if (result == result_code::rc_success) {
		stats_[mediaType].encrypt_success++;
	}
	else {
		stats_[mediaType].encrypt_failure++;
	}

	return result;
}

size_t encryptor::get_max_ciphertext_byte_size(media_type mediaType, size_t frameSize)
{
	return frameSize + SUPPLEMENTAL_BYTES + TRANSFORM_PADDING_BYTES;
}

void encryptor::assign_ssrc_to_codec(uint32_t ssrc, codec codecType)
{
	auto existingCodecIt = std::find_if(
	  ssrcCodecPairs_.begin(), ssrcCodecPairs_.end(), [ssrc](const SsrcCodecPair& pair) {
		  return pair.first == ssrc;
	  });

	if (existingCodecIt == ssrcCodecPairs_.end()) {
		ssrcCodecPairs_.emplace_back(ssrc, codecType);
	}
	else {
		existingCodecIt->second = codecType;
	}
}

codec encryptor::codec_for_ssrc(uint32_t ssrc)
{
	auto existingCodecIt = std::find_if(
	  ssrcCodecPairs_.begin(), ssrcCodecPairs_.end(), [ssrc](const SsrcCodecPair& pair) {
		  return pair.first == ssrc;
	  });

	if (existingCodecIt != ssrcCodecPairs_.end()) {
		return existingCodecIt->second;
	}
	else {
		return codec::cd_opus;
	}
}

std::unique_ptr<outbound_frame_processor> encryptor::get_or_create_frame_processor()
{
	std::lock_guard<std::mutex> lock(frameProcessorsMutex_);
	if (frameProcessors_.empty()) {
		return std::make_unique<outbound_frame_processor>(creator);
	}
	auto frameProcessor = std::move(frameProcessors_.back());
	frameProcessors_.pop_back();
	return frameProcessor;
}

void encryptor::return_frame_processor(std::unique_ptr<outbound_frame_processor> frameProcessor)
{
	std::lock_guard<std::mutex> lock(frameProcessorsMutex_);
	frameProcessors_.push_back(std::move(frameProcessor));
}

encryptor::cryptor_and_nonce encryptor::get_next_cryptor_and_nonce()
{
	std::lock_guard<std::mutex> lock(keyGenMutex_);
	if (!keyRatchet_) {
		return {nullptr, 0};
	}

	auto generation = compute_wrapped_generation(currentKeyGeneration_,
						 ++truncatedNonce_ >> RATCHET_GENERATION_SHIFT_BITS);

	if (generation != currentKeyGeneration_ || !cryptor_) {
		currentKeyGeneration_ = generation;

		auto encryptionKey = keyRatchet_->get_key(currentKeyGeneration_);
		cryptor_ = create_cipher(creator, encryptionKey);
	}

	return {cryptor_, truncatedNonce_};
}

void encryptor::update_current_protocol_version(protocol_version version)
{
	if (version == currentProtocolVersion_) {
		return;
	}

	currentProtocolVersion_ = version;
	if (protocolVersionChangedCallback_) {
		protocolVersionChangedCallback_();
	}
}

} // namespace dpp::dave

