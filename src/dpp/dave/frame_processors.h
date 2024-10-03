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
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "common.h"
#include "array_view.h"

namespace dpp::dave {

struct Range {
    size_t offset;
    size_t size;
};
using Ranges = std::vector<Range>;

uint8_t UnencryptedRangesSize(const Ranges& unencryptedRanges);
uint8_t SerializeUnencryptedRanges(const Ranges& unencryptedRanges,
                                   uint8_t* buffer,
                                   size_t bufferSize);
uint8_t DeserializeUnencryptedRanges(const uint8_t*& buffer,
                                     const size_t bufferSize,
                                     Ranges& unencryptedRanges);
bool ValidateUnencryptedRanges(const Ranges& unencryptedRanges, size_t frameSize);

class InboundFrameProcessor {
public:
    void ParseFrame(array_view<const uint8_t> frame);
    size_t ReconstructFrame(array_view<uint8_t> frame) const;

    bool IsEncrypted() const { return isEncrypted_; }
    size_t Size() const { return originalSize_; }
    void Clear();

    array_view<const uint8_t> GetTag() const { return tag_; }
    TruncatedSyncNonce GetTruncatedNonce() const { return truncatedNonce_; }
    array_view<const uint8_t> GetAuthenticatedData() const
    {
        return make_array_view(authenticated_.data(), authenticated_.size());
    }
    array_view<const uint8_t> GetCiphertext() const
    {
        return make_array_view(ciphertext_.data(), ciphertext_.size());
    }
    array_view<uint8_t> GetPlaintext() { return make_array_view(plaintext_); }

private:
    void AddAuthenticatedBytes(const uint8_t* data, size_t size);
    void AddCiphertextBytes(const uint8_t* data, size_t size);

    bool isEncrypted_{false};
    size_t originalSize_{0};
    array_view<const uint8_t> tag_;
    TruncatedSyncNonce truncatedNonce_;
    Ranges unencryptedRanges_;
    std::vector<uint8_t> authenticated_;
    std::vector<uint8_t> ciphertext_;
    std::vector<uint8_t> plaintext_;
};

class OutboundFrameProcessor {
public:
    void ProcessFrame(array_view<const uint8_t> frame, Codec codec);
    size_t ReconstructFrame(array_view<uint8_t> frame);

    Codec GetCodec() const { return codec_; }
    const std::vector<uint8_t>& GetUnencryptedBytes() const { return unencryptedBytes_; }
    const std::vector<uint8_t>& GetEncryptedBytes() const { return encryptedBytes_; }
    std::vector<uint8_t>& GetCiphertextBytes() { return ciphertextBytes_; }
    const Ranges& GetUnencryptedRanges() const { return unencryptedRanges_; }

    void Reset();
    void AddUnencryptedBytes(const uint8_t* bytes, size_t size);
    void AddEncryptedBytes(const uint8_t* bytes, size_t size);

private:
    Codec codec_{Codec::Unknown};
    size_t frameIndex_{0};
    std::vector<uint8_t> unencryptedBytes_;
    std::vector<uint8_t> encryptedBytes_;
    std::vector<uint8_t> ciphertextBytes_;
    Ranges unencryptedRanges_;
};

} // namespace dpp::dave

