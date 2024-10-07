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

namespace dpp {
	class cluster;
}

namespace dpp::dave {

/**
 * @brief Range inside a frame
 */
struct range {
	size_t offset;
	size_t size;
};

/**
 * @brief Vector of ranges in a frame
 */
using ranges = std::vector<range>;

/**
 * @brief Get total size of unencrypted ranges
 * @param unencryptedRanges unencrypted ranges
 * @return size
 */
uint8_t unencrypted_ranges_size(const ranges& unencryptedRanges);

/**
 * @brief Serialise unencrypted ranges
 * @param unencryptedRanges unencrypted ranges
 * @param buffer buffer to serialise to
 * @param bufferSize size of buffer
 * @return size of ranges written
 */
uint8_t serialize_unencrypted_ranges(const ranges& unencryptedRanges, uint8_t* buffer, size_t bufferSize);

/**
 * @brief Deserialise unencrypted ranges
 * @param buffer buffer to write to
 * @param bufferSize buffer size
 * @param unencryptedRanges unencrypted ranges to write to
 * @return size of unencrypted ranges written
 */
uint8_t deserialize_unencrypted_ranges(const uint8_t*& buffer, const size_t bufferSize, ranges& unencryptedRanges);

/**
 * @brief Validate unencrypted ranges
 * @param unencryptedRanges unencrypted ranges
 * @param frameSize frame size
 * @return true if validated
 */
bool validate_unencrypted_ranges(const ranges& unencryptedRanges, size_t frameSize);

/**
 * @brief Processes inbound frames from the decryptor
 */
class inbound_frame_processor {
public:
	/**
	 * @brief Create inbound frame processor
	 * @param _creator creating cluster
	 */
	inbound_frame_processor(dpp::cluster& _creator) : creator(_creator) { };

	/**
	 * @brief Parse inbound frame
	 * @param frame frame bytes
	 */
	void parse_frame(array_view<const uint8_t> frame);

	/**
	 * @brief Rebuild frame after decryption
	 * @param frame frame bytes
	 * @return size of reconstructed frame
	 */
	[[nodiscard]] size_t reconstruct_frame(array_view<uint8_t> frame) const;

	/**
	 * @brief True if encrypted
	 * @return is encrypted
	 */
	[[nodiscard]] bool is_encrypted() const {
		return isEncrypted_;
	}

	/**
	 * @brief Get size
	 * @return Original frame size
	 */
	[[nodiscard]] size_t size() const {
		return originalSize_;
	}

	/**
	 * @brief Clear the processor state
	 */
	void clear();

	/**
	 * @brief get AEAD tag for frame processor
	 * @return AEAD tag
	 */
	[[nodiscard]] array_view<const uint8_t> get_tag() const {
		return tag_;
	}

	/**
	 * @brief Get truncated sync nonce
	 * @return truncated sync nonce
	 */
	[[nodiscard]] truncated_sync_nonce get_truncated_nonce() const {
		return truncatedNonce_;
	}

	/**
	 * @brief Get authenticated AEAD data
	 * @return AEAD auth data
	 */
	[[nodiscard]] array_view<const uint8_t> get_authenticated_data() const {
		return make_array_view(authenticated_.data(), authenticated_.size());
	}

	/**
	 * @brief Get ciphertext
	 * @return Ciphertext view
	 */
	[[nodiscard]] array_view<const uint8_t> get_ciphertext() const {
		return make_array_view(ciphertext_.data(), ciphertext_.size());
	}

	/**
	 * @brief Get plain text
	 * @return Plain text view
	 */
	[[nodiscard]] array_view<uint8_t> get_plaintext() { return make_array_view(plaintext_); }

private:
	/**
	 * @brief Add authenticated bytes
	 * @param data authenticated data
	 * @param size authenticated data size
	 */
	void add_authenticated_bytes(const uint8_t* data, size_t size);

	/**
	 * @brief Add ciphertext bytes
	 * @param data ciphertext data
	 * @param size ciphertext data size
	 */
	void add_ciphertext_bytes(const uint8_t* data, size_t size);

	bool isEncrypted_{false};
	size_t originalSize_{0};
	array_view<const uint8_t> tag_;
	truncated_sync_nonce truncatedNonce_;
	ranges unencryptedRanges_;
	std::vector<uint8_t> authenticated_;
	std::vector<uint8_t> ciphertext_;
	std::vector<uint8_t> plaintext_;

	/**
	 * @brief DPP Cluster, used for logging
	 */
	dpp::cluster& creator;
};

/**
 * @brief Outbound frame processor, processes outbound frames for encryption
 */
class outbound_frame_processor {
public:
	/**
	 * @brief Create outbound frame processor
	 * @param _creator creating cluster
	 */
	outbound_frame_processor(dpp::cluster& _creator) : creator(_creator) { };

	/**
	 * @brief Process outbound frame
	 * @param frame frame data
	 * @param codec codec to use
	 */
	void process_frame(array_view<const uint8_t> frame, codec codec);

	/**
	 * @brief Reconstruct frame
	 * @param frame frame data
	 * @return size of reconstructed frame
	 */
	size_t reconstruct_frame(array_view<uint8_t> frame);

	/**
	 * @brief Get codec
	 * @return codec
	 */
	[[nodiscard]] codec get_codec() const {
		return codec_;
	}

	/**
	 * @brief Get unencrypted bytes
	 * @return unencrypted bytes
	 */
	[[nodiscard]] const std::vector<uint8_t>& get_unencrypted_bytes() const {
		return unencryptedBytes_;
	}

	/**
	 * @brief Get encrypted bytes
	 * @return Encrypted bytes
	 */
	[[nodiscard]] const std::vector<uint8_t>& get_encrypted_bytes() const {
		return encryptedBytes_;
	}

	/**
	 * @brief Get ciphertext bytes
	 * @return ciphertext bytes
	 */
	[[nodiscard]] std::vector<uint8_t>& get_ciphertext_bytes() {
		return ciphertextBytes_;
	}

	/**
	 * @brief Get unencrypted bytes
	 * @return unencrypted bytes
	 */
	[[nodiscard]] const ranges& get_unencrypted_ranges() const {
		return unencryptedRanges_;
	}

	/**
	 * @brief Reset outbound processor
	 */
	void reset();

	/**
	 * @brief Add unencrypted bytes
	 * @param bytes unencrypted bytes
	 * @param size unencrypted size
	 */
	void add_unencrypted_bytes(const uint8_t* bytes, size_t size);

	/**
	 * @brief Add encrypted bytes
	 * @param bytes encrypted bytes
	 * @param size encrypted size
	 */
	void add_encrypted_bytes(const uint8_t* bytes, size_t size);

private:
	codec codec_{codec::cd_unknown};
	size_t frameIndex_{0};
	std::vector<uint8_t> unencryptedBytes_;
	std::vector<uint8_t> encryptedBytes_;
	std::vector<uint8_t> ciphertextBytes_;
	ranges unencryptedRanges_;

	/**
	 * @brief DPP Cluster, used for logging
	 */
	dpp::cluster& creator;
};

} // namespace dpp::dave

