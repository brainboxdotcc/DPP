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

struct range {
	size_t offset;
	size_t size;
};
using ranges = std::vector<range>;

uint8_t unencrypted_ranges_size(const ranges& unencryptedRanges);
uint8_t serialize_unencrypted_ranges(const ranges& unencryptedRanges, uint8_t* buffer, size_t bufferSize);
uint8_t deserialize_unencrypted_ranges(const uint8_t*& buffer, const size_t bufferSize, ranges& unencryptedRanges);
bool validate_unencrypted_ranges(const ranges& unencryptedRanges, size_t frameSize);

class inbound_frame_processor {
public:
	void parse_frame(array_view<const uint8_t> frame);
	[[nodiscard]] size_t reconstruct_frame(array_view<uint8_t> frame) const;

	[[nodiscard]] bool is_encrypted() const { return isEncrypted_; }
	[[nodiscard]] size_t size() const { return originalSize_; }
	void clear();

	[[nodiscard]] array_view<const uint8_t> get_tag() const { return tag_; }
	[[nodiscard]] truncated_sync_nonce get_truncated_nonce() const { return truncatedNonce_; }
	[[nodiscard]] array_view<const uint8_t> get_authenticated_data() const
	{
		return make_array_view(authenticated_.data(), authenticated_.size());
	}
	[[nodiscard]] array_view<const uint8_t> GetCiphertext() const
	{
		return make_array_view(ciphertext_.data(), ciphertext_.size());
	}
	[[nodiscard]] array_view<uint8_t> get_plaintext() { return make_array_view(plaintext_); }

private:
	void add_authenticated_bytes(const uint8_t* data, size_t size);
	void add_ciphertext_bytes(const uint8_t* data, size_t size);

	bool isEncrypted_{false};
	size_t originalSize_{0};
	array_view<const uint8_t> tag_;
	truncated_sync_nonce truncatedNonce_;
	ranges unencryptedRanges_;
	std::vector<uint8_t> authenticated_;
	std::vector<uint8_t> ciphertext_;
	std::vector<uint8_t> plaintext_;
};

class outbound_frame_processor {
public:
	void process_frame(array_view<const uint8_t> frame, codec codec);
	size_t reconstruct_frame(array_view<uint8_t> frame);

	[[nodiscard]] codec get_codec() const { return codec_; }
	[[nodiscard]] const std::vector<uint8_t>& get_unencrypted_bytes() const { return unencryptedBytes_; }
	[[nodiscard]] const std::vector<uint8_t>& get_encrypted_bytes() const { return encryptedBytes_; }
	[[nodiscard]] std::vector<uint8_t>& get_ciphertext_bytes() { return ciphertextBytes_; }
	[[nodiscard]] const ranges& get_unencrypted_ranges() const { return unencryptedRanges_; }

	void reset();
	void add_unencrypted_bytes(const uint8_t* bytes, size_t size);
	void add_encrypted_bytes(const uint8_t* bytes, size_t size);

private:
	codec codec_{codec::Unknown};
	size_t frameIndex_{0};
	std::vector<uint8_t> unencryptedBytes_;
	std::vector<uint8_t> encryptedBytes_;
	std::vector<uint8_t> ciphertextBytes_;
	ranges unencryptedRanges_;
};

} // namespace dpp::dave

