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
#include "common.h"
#include "cryptor_manager.h"
#include "logger.h"
#include "codec_utils.h"
#include "array_view.h"
#include "leb128.h"
#include "scope_exit.h"

using namespace std::chrono_literals;

namespace dpp::dave {

constexpr auto kStatsInterval = 10s;

void encryptor::set_key_ratchet(std::unique_ptr<IKeyRatchet> keyRatchet)
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
	update_current_protocol_version(passthroughMode ? 0 : MaxSupportedProtocolVersion());
}

int encryptor::encrypt(media_type mediaType,
		       uint32_t ssrc,
		       array_view<const uint8_t> frame,
		       array_view<uint8_t> encryptedFrame,
		       size_t* bytesWritten)
{
    if (mediaType != media_audio && mediaType != media_video) {
        DISCORD_LOG(LS_WARNING) << "encrypt failed, invalid media type: "
                                << static_cast<int>(mediaType);
        return 0;
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
    ScopeExit cleanup([&] { return_frame_processor(std::move(frameProcessor)); });

    frameProcessor->ProcessFrame(frame, codec);

    const auto& unencryptedBytes = frameProcessor->GetUnencryptedBytes();
    const auto& encryptedBytes = frameProcessor->GetEncryptedBytes();
    auto& ciphertextBytes = frameProcessor->GetCiphertextBytes();

    const auto& unencryptedRanges = frameProcessor->GetUnencryptedRanges();
    auto unencryptedRangesSize = UnencryptedRangesSize(unencryptedRanges);

    auto additionalData = make_array_view(unencryptedBytes.data(), unencryptedBytes.size());
    auto plaintextBuffer = make_array_view(encryptedBytes.data(), encryptedBytes.size());
    auto ciphertextBuffer = make_array_view(ciphertextBytes.data(), ciphertextBytes.size());

    auto frameSize = encryptedBytes.size() + unencryptedBytes.size();
    auto tagBuffer = make_array_view(encryptedFrame.data() + frameSize, kAesGcm128TruncatedTagBytes);

    auto nonceBuffer = std::array<uint8_t, kAesGcm128NonceBytes>();
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
        std::memcpy(nonceBuffer.data() + kAesGcm128TruncatedSyncNonceOffset,
               &truncatedNonce,
               kAesGcm128TruncatedSyncNonceBytes);

        // encrypt the plaintext, adding the unencrypted header to the tag
        bool success = cryptor->encrypt(
		ciphertextBuffer, plaintextBuffer, nonceBufferView, additionalData, tagBuffer);

        stats_[mediaType].encrypt_attempts++;
        stats_[mediaType].encrypt_max_attempts =
          std::max(stats_[mediaType].encrypt_max_attempts, (uint64_t)attempt);

        if (!success) {
            assert(false && "Failed to encrypt frame");
            result = result_code::rc_encryption_failure;
            break;
        }

        auto reconstructedFrameSize = frameProcessor->ReconstructFrame(encryptedFrame);
        assert(reconstructedFrameSize == frameSize && "Failed to reconstruct frame");

        auto nonceSize = Leb128Size(truncatedNonce);

        auto truncatedNonceBuffer = make_array_view(tagBuffer.end(), nonceSize);
        auto unencryptedRangesBuffer =
		make_array_view(truncatedNonceBuffer.end(), unencryptedRangesSize);
        auto supplementalBytesBuffer =
		make_array_view(unencryptedRangesBuffer.end(), sizeof(supplemental_bytes_size));
        auto markerBytesBuffer = make_array_view(supplementalBytesBuffer.end(), sizeof(magic_marker));

        // write the nonce
        auto res = WriteLeb128(truncatedNonce, truncatedNonceBuffer.begin());
        if (res != nonceSize) {
            assert(false && "Failed to write truncated nonce");
            result = result_code::rc_encryption_failure;
            break;
        }

        // write the unencrypted ranges
        res = SerializeUnencryptedRanges(
          unencryptedRanges, unencryptedRangesBuffer.begin(), unencryptedRangesBuffer.size());
        if (res != unencryptedRangesSize) {
            assert(false && "Failed to write unencrypted ranges");
            result = result_code::rc_encryption_failure;
            break;
        }

        // write the supplemental bytes size
        supplemental_bytes_size supplementalBytes =
          kSupplementalBytes + nonceSize + unencryptedRangesSize;
	std::memcpy(supplementalBytesBuffer.data(), &supplementalBytes, sizeof(supplemental_bytes_size));

        // write the marker bytes, ends the frame
	std::memcpy(markerBytesBuffer.data(), &kMarkerBytes, sizeof(magic_marker));

        auto encryptedFrameBytes = reconstructedFrameSize + kAesGcm128TruncatedTagBytes +
				   nonceSize + unencryptedRangesSize + sizeof(supplemental_bytes_size) + sizeof(magic_marker);

        if (codec_utils::validate_encrypted_frame(
		*frameProcessor, make_array_view(encryptedFrame.data(), encryptedFrameBytes))) {
            *bytesWritten = encryptedFrameBytes;
            break;
        }
        else if (attempt >= MAX_CIPHERTEXT_VALIDATION_RETRIES) {
            assert(false && "Failed to validate encrypted section for codec");
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

    if (now > lastStatsTime_ + kStatsInterval) {
        lastStatsTime_ = now;
        DISCORD_LOG(LS_INFO) << "Encrypted audio: " << stats_[media_audio].encrypt_success
                             << ", video: " << stats_[media_video].encrypt_success
                             << ". Failed audio: " << stats_[media_audio].encrypt_failure
                             << ", video: " << stats_[media_video].encrypt_failure;
        DISCORD_LOG(LS_INFO) << "Last encrypted frame, type: "
                             << (mediaType == media_audio ? "audio" : "video") << ", ssrc: " << ssrc
                             << ", size: " << frame.size();
    }

    return result;
}

size_t encryptor::get_max_ciphertext_byte_size(media_type mediaType, size_t frameSize)
{
    return frameSize + kSupplementalBytes + kTransformPaddingBytes;
}

void encryptor::assign_ssrc_to_codec(uint32_t ssrc, Codec codecType)
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

Codec encryptor::codec_for_ssrc(uint32_t ssrc)
{
    auto existingCodecIt = std::find_if(
      ssrcCodecPairs_.begin(), ssrcCodecPairs_.end(), [ssrc](const SsrcCodecPair& pair) {
          return pair.first == ssrc;
      });

    if (existingCodecIt != ssrcCodecPairs_.end()) {
        return existingCodecIt->second;
    }
    else {
        return Codec::Opus;
    }
}

std::unique_ptr<OutboundFrameProcessor> encryptor::get_or_create_frame_processor()
{
    std::lock_guard<std::mutex> lock(frameProcessorsMutex_);
    if (frameProcessors_.empty()) {
        return std::make_unique<OutboundFrameProcessor>();
    }
    auto frameProcessor = std::move(frameProcessors_.back());
    frameProcessors_.pop_back();
    return frameProcessor;
}

void encryptor::return_frame_processor(std::unique_ptr<OutboundFrameProcessor> frameProcessor)
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
						 ++truncatedNonce_ >> kRatchetGenerationShiftBits);

    if (generation != currentKeyGeneration_ || !cryptor_) {
        currentKeyGeneration_ = generation;

        auto encryptionKey = keyRatchet_->GetKey(currentKeyGeneration_);
        cryptor_ = create_cipher(encryptionKey);
    }

    return {cryptor_, truncatedNonce_};
}

void encryptor::update_current_protocol_version(ProtocolVersion version)
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

