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
#include <cassert>
#include <limits>
#include <optional>
#include <memory>
#include <cstring>
#include "codec_utils.h"
#include "logger.h"
#include "array_view.h"
#include "leb128.h"

#if defined(_MSC_VER)
	#include <intrin.h>
#endif

namespace dpp::dave {

std::pair<bool, size_t> OverflowAdd(size_t a, size_t b)
{
    size_t res;
#if defined(_MSC_VER) && defined(_M_X64)
    bool didOverflow = _addcarry_u64(0, a, b, &res);
#elif defined(_MSC_VER) && defined(_M_IX86)
    bool didOverflow = _addcarry_u32(0, a, b, &res);
#else
    bool didOverflow = __builtin_add_overflow(a, b, &res);
#endif
    return {didOverflow, res};
}

uint8_t UnencryptedRangesSize(const Ranges& unencryptedRanges)
{
    size_t size = 0;
    for (const auto& range : unencryptedRanges) {
        size += Leb128Size(range.offset);
        size += Leb128Size(range.size);
    }
    assert(size <= std::numeric_limits<uint8_t>::max() &&
           "Unencrypted ranges size exceeds 255 bytes");
    return static_cast<uint8_t>(size);
}

uint8_t SerializeUnencryptedRanges(const Ranges& unencryptedRanges,
                                   uint8_t* buffer,
                                   size_t bufferSize)
{
    auto writeAt = buffer;
    auto end = buffer + bufferSize;
    for (const auto& range : unencryptedRanges) {
        auto rangeSize = Leb128Size(range.offset) + Leb128Size(range.size);
        if (rangeSize > static_cast<size_t>(end - writeAt)) {
            assert(false && "Buffer is too small to serialize unencrypted ranges");
            break;
        }

        writeAt += WriteLeb128(range.offset, writeAt);
        writeAt += WriteLeb128(range.size, writeAt);
    }
    return writeAt - buffer;
}

uint8_t DeserializeUnencryptedRanges(const uint8_t*& readAt,
                                     const size_t bufferSize,
                                     Ranges& unencryptedRanges)
{
    auto start = readAt;
    auto end = readAt + bufferSize;
    while (readAt < end) {
        size_t offset = ReadLeb128(readAt, end);
        if (readAt == nullptr) {
            break;
        }

        size_t size = ReadLeb128(readAt, end);
        if (readAt == nullptr) {
            break;
        }
        unencryptedRanges.push_back({offset, size});
    }

    if (readAt != end) {
        DISCORD_LOG(LS_WARNING) << "Failed to deserialize unencrypted ranges";
        unencryptedRanges.clear();
        readAt = nullptr;
        return 0;
    }

    return readAt - start;
}

bool ValidateUnencryptedRanges(const Ranges& unencryptedRanges, size_t frameSize)
{
    if (unencryptedRanges.empty()) {
        return true;
    }

    // validate that the ranges are in order and don't overlap
    for (auto i = 0u; i < unencryptedRanges.size(); ++i) {
        auto current = unencryptedRanges[i];
        // The current range should not overflow into the next range
        // or if it is the last range, the end of the frame
        auto maxEnd =
          i + 1 < unencryptedRanges.size() ? unencryptedRanges[i + 1].offset : frameSize;

        auto [didOverflow, currentEnd] = OverflowAdd(current.offset, current.size);
        if (didOverflow || currentEnd > maxEnd) {
            DISCORD_LOG(LS_WARNING)
              << "Unencrypted range may overlap or be out of order: current offset: "
              << current.offset << ", current size: " << current.size << ", maximum end: " << maxEnd
              << ", frame size: " << frameSize;
            return false;
        }
    }

    return true;
}

size_t Reconstruct(Ranges ranges,
                   const std::vector<uint8_t>& rangeBytes,
                   const std::vector<uint8_t>& otherBytes,
                   const array_view<uint8_t>& output)
{
    size_t frameIndex = 0;
    size_t rangeBytesIndex = 0;
    size_t otherBytesIndex = 0;

    const auto CopyRangeBytes = [&](size_t size) {
        assert(rangeBytesIndex + size <= rangeBytes.size());
        assert(frameIndex + size <= output.size());
	std::memcpy(output.data() + frameIndex, rangeBytes.data() + rangeBytesIndex, size);
        rangeBytesIndex += size;
        frameIndex += size;
    };

    const auto CopyOtherBytes = [&](size_t size) {
        assert(otherBytesIndex + size <= otherBytes.size());
        assert(frameIndex + size <= output.size());
        std::memcpy(output.data() + frameIndex, otherBytes.data() + otherBytesIndex, size);
        otherBytesIndex += size;
        frameIndex += size;
    };

    for (const auto& range : ranges) {
        if (range.offset > frameIndex) {
            CopyOtherBytes(range.offset - frameIndex);
        }

        CopyRangeBytes(range.size);
    }

    if (otherBytesIndex < otherBytes.size()) {
        CopyOtherBytes(otherBytes.size() - otherBytesIndex);
    }

    assert(rangeBytesIndex == rangeBytes.size());
    assert(otherBytesIndex == otherBytes.size());
    assert(frameIndex <= output.size());

    return frameIndex;
}

void inbound_frame_processor::Clear()
{
    isEncrypted_ = false;
    originalSize_ = 0;
    truncatedNonce_ = std::numeric_limits<truncated_sync_nonce>::max();
    unencryptedRanges_.clear();
    authenticated_.clear();
    ciphertext_.clear();
    plaintext_.clear();
}

void inbound_frame_processor::ParseFrame(array_view<const uint8_t> frame)
{
    Clear();

    constexpr auto MinSupplementalBytesSize =
	    kAesGcm128TruncatedTagBytes + sizeof(supplemental_bytes_size) + sizeof(magic_marker);
    if (frame.size() < MinSupplementalBytesSize) {
        DISCORD_LOG(LS_WARNING) << "Encrypted frame is too small to contain min supplemental bytes";
        return;
    }

    // Check the frame ends with the magic marker
    auto magicMarkerBuffer = frame.end() - sizeof(magic_marker);
    if (memcmp(magicMarkerBuffer, &kMarkerBytes, sizeof(magic_marker)) != 0) {
        return;
    }

    // Read the supplemental bytes size
    supplemental_bytes_size supplementalBytesSize;
    auto supplementalBytesSizeBuffer = magicMarkerBuffer - sizeof(supplemental_bytes_size);
    assert(frame.begin() <= supplementalBytesSizeBuffer &&
           supplementalBytesSizeBuffer <= frame.end());
    memcpy(&supplementalBytesSize, supplementalBytesSizeBuffer, sizeof(supplemental_bytes_size));

    // Check the frame is large enough to contain the supplemental bytes
    if (frame.size() < supplementalBytesSize) {
        DISCORD_LOG(LS_WARNING) << "Encrypted frame is too small to contain supplemental bytes";
        return;
    }

    // Check that supplemental bytes size is large enough to contain the supplemental bytes
    if (supplementalBytesSize < MinSupplementalBytesSize) {
        DISCORD_LOG(LS_WARNING)
          << "Supplemental bytes size is too small to contain supplemental bytes";
        return;
    }

    auto supplementalBytesBuffer = frame.end() - supplementalBytesSize;
    assert(frame.begin() <= supplementalBytesBuffer && supplementalBytesBuffer <= frame.end());

    // Read the tag
    tag_ = make_array_view(supplementalBytesBuffer, kAesGcm128TruncatedTagBytes);

    // Read the nonce
    auto nonceBuffer = supplementalBytesBuffer + kAesGcm128TruncatedTagBytes;
    assert(frame.begin() <= nonceBuffer && nonceBuffer <= frame.end());
    auto readAt = nonceBuffer;
    auto end = supplementalBytesSizeBuffer;
    truncatedNonce_ = ReadLeb128(readAt, end);
    if (readAt == nullptr) {
        DISCORD_LOG(LS_WARNING) << "Failed to read truncated nonce";
        return;
    }

    // Read the unencrypted ranges
    assert(nonceBuffer <= readAt && readAt <= end);
    auto unencryptedRangesSize = end - readAt;
    DeserializeUnencryptedRanges(readAt, unencryptedRangesSize, unencryptedRanges_);
    if (readAt == nullptr) {
        DISCORD_LOG(LS_WARNING) << "Failed to read unencrypted ranges";
        return;
    }

    if (!ValidateUnencryptedRanges(unencryptedRanges_, frame.size())) {
        DISCORD_LOG(LS_WARNING) << "Invalid unencrypted ranges";
        return;
    }

    // This is overly aggressive but will keep reallocations to a minimum
    authenticated_.reserve(frame.size());
    ciphertext_.reserve(frame.size());
    plaintext_.reserve(frame.size());

    originalSize_ = frame.size();

    // Split the frame into authenticated and ciphertext bytes
    size_t frameIndex = 0;
    for (const auto& range : unencryptedRanges_) {
        auto encryptedBytes = range.offset - frameIndex;
        if (encryptedBytes > 0) {
            assert(frameIndex + encryptedBytes <= frame.size());
            AddCiphertextBytes(frame.data() + frameIndex, encryptedBytes);
        }

        assert(range.offset + range.size <= frame.size());
        AddAuthenticatedBytes(frame.data() + range.offset, range.size);
        frameIndex = range.offset + range.size;
    }
    auto actualFrameSize = frame.size() - supplementalBytesSize;
    if (frameIndex < actualFrameSize) {
        AddCiphertextBytes(frame.data() + frameIndex, actualFrameSize - frameIndex);
    }

    // Make sure the plaintext buffer is the same size as the ciphertext buffer
    plaintext_.resize(ciphertext_.size());

    // We've successfully parsed the frame
    // Mark the frame as encrypted
    isEncrypted_ = true;
}

size_t inbound_frame_processor::ReconstructFrame(array_view<uint8_t> frame) const
{
    if (!isEncrypted_) {
        DISCORD_LOG(LS_WARNING) << "Cannot reconstruct an invalid encrypted frame";
        return 0;
    }

    if (authenticated_.size() + plaintext_.size() > frame.size()) {
        DISCORD_LOG(LS_WARNING) << "Frame is too small to contain the decrypted frame";
        return 0;
    }

    return Reconstruct(unencryptedRanges_, authenticated_, plaintext_, frame);
}

void inbound_frame_processor::AddAuthenticatedBytes(const uint8_t* data, size_t size)
{
    authenticated_.resize(authenticated_.size() + size);
    memcpy(authenticated_.data() + authenticated_.size() - size, data, size);
}

void inbound_frame_processor::AddCiphertextBytes(const uint8_t* data, size_t size)
{
    ciphertext_.resize(ciphertext_.size() + size);
    memcpy(ciphertext_.data() + ciphertext_.size() - size, data, size);
}

void OutboundFrameProcessor::Reset()
{
    codec_ = Codec::Unknown;
    frameIndex_ = 0;
    unencryptedBytes_.clear();
    encryptedBytes_.clear();
    unencryptedRanges_.clear();
}

void OutboundFrameProcessor::ProcessFrame(array_view<const uint8_t> frame, Codec codec)
{
    Reset();

    codec_ = codec;
    unencryptedBytes_.reserve(frame.size());
    encryptedBytes_.reserve(frame.size());

    bool success = false;
    switch (codec) {
    case Codec::Opus:
        success = codec_utils::process_frame_opus(*this, frame);
        break;
    case Codec::VP8:
        success = codec_utils::process_frame_vp8(*this, frame);
        break;
    case Codec::VP9:
        success = codec_utils::process_frame_vp9(*this, frame);
        break;
    case Codec::H264:
        success = codec_utils::process_frame_h264(*this, frame);
        break;
    case Codec::H265:
        success = codec_utils::process_frame_h265(*this, frame);
        break;
    case Codec::AV1:
        success = codec_utils::process_frame_av1(*this, frame);
        break;
    default:
        assert(false && "Unsupported codec for frame encryption");
        break;
    }

    if (!success) {
        frameIndex_ = 0;
        unencryptedBytes_.clear();
        encryptedBytes_.clear();
        unencryptedRanges_.clear();
        AddEncryptedBytes(frame.data(), frame.size());
    }

    ciphertextBytes_.resize(encryptedBytes_.size());
}

size_t OutboundFrameProcessor::ReconstructFrame(array_view<uint8_t> frame)
{
    if (unencryptedBytes_.size() + ciphertextBytes_.size() > frame.size()) {
        DISCORD_LOG(LS_WARNING) << "Frame is too small to contain the encrypted frame";
        return 0;
    }

    return Reconstruct(unencryptedRanges_, unencryptedBytes_, ciphertextBytes_, frame);
}

void OutboundFrameProcessor::AddUnencryptedBytes(const uint8_t* bytes, size_t size)
{
    if (!unencryptedRanges_.empty() &&
        unencryptedRanges_.back().offset + unencryptedRanges_.back().size == frameIndex_) {
        // extend the last range
        unencryptedRanges_.back().size += size;
    }
    else {
        // add a new range (offset, size)
        unencryptedRanges_.push_back({frameIndex_, size});
    }

    unencryptedBytes_.resize(unencryptedBytes_.size() + size);
    memcpy(unencryptedBytes_.data() + unencryptedBytes_.size() - size, bytes, size);
    frameIndex_ += size;
}

void OutboundFrameProcessor::AddEncryptedBytes(const uint8_t* bytes, size_t size)
{
    encryptedBytes_.resize(encryptedBytes_.size() + size);
    memcpy(encryptedBytes_.data() + encryptedBytes_.size() - size, bytes, size);
    frameIndex_ += size;
}

} // namespace dpp::dave

