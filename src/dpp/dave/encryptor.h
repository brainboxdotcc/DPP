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

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include "codec_utils.h"
#include "common.h"
#include "cipher_interface.h"
#include "key_ratchet.h"
#include "frame_processors.h"
#include "version.h"

namespace dpp {
	class cluster;
}

namespace dpp::dave {

/**
 * @brief Encryption stats
 */
struct encryption_stats {
	/**
	 * @brief Number of passthrough packets
	 */
	uint64_t passthroughs = 0;
	/**
	 * @brief Number of encryption successes
	 */
	uint64_t encrypt_success = 0;
	/**
	 * @brief Number of encryption failures
	 */
	uint64_t encrypt_failure = 0;
	/**
	 * @brief Duration encrypted
	 */
	uint64_t encrypt_duration = 0;
	/**
	 * @brief Number of encryption atempts
	 */
	uint64_t encrypt_attempts = 0;
	/**
	 * @brief Maximum attempts at encryption
	 */
	uint64_t encrypt_max_attempts = 0;
};

class encryptor {
public:
	/**
	 * @brief Constructor
	 * @param cl Creator
	 */
	encryptor(dpp::cluster& cl) : creator(cl) { };

	/**
	 * @brief Return codes for encryptor::encrypt
	 */
	enum result_code : uint8_t {
		/**
		 * @brief Successful encryption
		 */
		rc_success,
		/**
		 * @brief Encryption failure
		 */
		rc_encryption_failure,
	};

	/**
	 * @brief Set key ratchet for encryptor, this should be the bot's ratchet.
	 * @param key_ratchet Bot's key ratchet
	 */
	void set_key_ratchet(std::unique_ptr<key_ratchet_interface> key_ratchet);

	/**
	 * @brief Set encryption to passthrough mode
	 * @param passthrough_mode true to enable passthrough mode, false to disable
	 */
	void set_passthrough_mode(bool passthrough_mode);

	/**
	 * @brief True if key ratchet assigned
	 * @return key ratchet is assigned
	 */
	bool has_key_ratchet() const {
		return ratchet != nullptr;
	}

	/**
	 * @brief True if is in passthrough mode
	 * @return is in passthrough mode
	 */
	bool is_passthrough_mode() const {
		return passthrough_mode_enable;
	}

	/**
	 * @brief Assign SSRC to codec
	 * @note This is unused - all SSRC are assumed to be OPUS for bots at present.
	 * @param ssrc RTP SSRC
	 * @param codec_type Codec type
	 */
	void assign_ssrc_to_codec(uint32_t ssrc, codec codec_type);

	/**
	 * @brief Get codec for RTP SSRC
	 * @note This is unused - all SSRC are assumed to be OPUS for bots at present.
	 * @param ssrc RTP SSRC
	 * @return always returns OPUS as bots can only send/receive audio at present
	 */
	codec codec_for_ssrc(uint32_t ssrc);

	/**
	 * @brief Encrypt plaintext opus frames
	 * @param this_media_type media type, should always be audio
	 * @param ssrc RTP SSRC
	 * @param frame Frame plaintext
	 * @param encrypted_frame Encrypted frame
	 * @param bytes_written Number of bytes written to the encrypted buffer
	 * @return Status code for encryption
	 */
	encryptor::result_code encrypt(media_type this_media_type, uint32_t ssrc, array_view<const uint8_t> frame, array_view<uint8_t> encrypted_frame, size_t* bytes_written);

	/**
	 * @brief Get maximum possible ciphertext size for a plaintext buffer
	 * @param this_media_type media type, should always be audio for bots
	 * @param frame_size frame size of plaintext buffer
	 * @return size of ciphertext buffer to allocate
	 */
	size_t get_max_ciphertext_byte_size(media_type this_media_type, size_t frame_size);

	/**
	 * @brief Get encryption stats
	 * @param this_media_type media type
	 * @return encryption stats
	 */
	encryption_stats get_stats(media_type this_media_type) const {
		return stats[this_media_type];
	}

	/**
	 * @brief Protocol version changed callback
	 */
	using protocol_version_changed_callback = std::function<void()>;

	/**
	 * @brief Set protocol version changed callback
	 * @param callback Callback to set
	 */
	void set_protocol_version_changed_callback(protocol_version_changed_callback callback) {
		changed_callback = std::move(callback);
	}

	/**
	 * @brief Get protocol version
	 * @return protocol version
	 */
	protocol_version get_protocol_version() const {
		return current_protocol_version;
	}

private:
	/**
	 * @brief Get the current frame processor or create a new one
	 * @return Frame processor
	 */
	std::unique_ptr<outbound_frame_processor> get_or_create_frame_processor();

	/**
	 * @brief Return frame processor
	 * @param frameProcessor frame processor
	 */
	void return_frame_processor(std::unique_ptr<outbound_frame_processor> frameProcessor);

	/**
	 * @brief Pair of cryptor and nonce pointers
	 */
	using cryptor_and_nonce = std::pair<std::shared_ptr<cipher_interface>, truncated_sync_nonce>;

	/**
	 * @brief Get cryptor and nonce
	 * @return cryptor and nonce
	 */
	cryptor_and_nonce get_next_cryptor_and_nonce();

	/**
	 * @brief Change protocol version
	 * @param version new protocol version
	 */
	void update_current_protocol_version(protocol_version version);

	/**
	 * @brief True if passthrough is enabled
	 */
	std::atomic_bool passthrough_mode_enable{false};

	/**
	 * @brief Key generation mutex for thread safety
	 */
	std::mutex key_gen_mutex;

	/**
	 * @brief Current encryption (send) ratchet
	 */
	std::unique_ptr<key_ratchet_interface> ratchet;

	/**
	 * @brief Current encryption cipher
	 */
	std::shared_ptr<cipher_interface> cryptor;

	/**
	 * @brief Current key generation number
	 */
	key_generation current_key_generation{0};

	/**
	 * @brief Current truncated sync nonce
	 */
	truncated_sync_nonce truncated_nonce{0};

	/**
	 * @brief Frame processor list mutex
	 */
	std::mutex frame_processors_mutex;

	/**
	 * @brief List of outbound frame processors
	 */
	std::vector<std::unique_ptr<outbound_frame_processor>> frame_processors;

	/**
	 * @brief A pair of 32 bit SSRC and codec in use for that SSRC
	 */
	using ssrc_codec_pair = std::pair<uint32_t, codec>;

	/**
	 * @brief List of codec pairs for SSRCs
	 */
	std::vector<ssrc_codec_pair> ssrc_codec_pairs;

	/**
	 * @brief A chrono time point
	 */
	using time_point = std::chrono::time_point<std::chrono::steady_clock>;

	/**
	 * @brief Last time stats were updated
	 */
	time_point last_stats_time{time_point::min()};

	/**
	 * @brief Stores audio/video encryption stats
	 */
	std::array<encryption_stats, 2> stats;

	/**
	 * @brief Callback for version change, if any
	 */
	protocol_version_changed_callback changed_callback;

	/**
	 * Current protocol version supported
	 */
	protocol_version current_protocol_version{max_protocol_version()};

	/**
	 * @brief DPP Cluster, used for logging
	 */
	dpp::cluster& creator;
};

}
