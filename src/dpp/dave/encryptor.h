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

namespace dpp::dave {

struct encryption_stats {
	uint64_t passthroughs = 0;
	uint64_t encrypt_success = 0;
	uint64_t encrypt_failure = 0;
	uint64_t encrypt_duration = 0;
	uint64_t encrypt_attempts = 0;
	uint64_t encrypt_max_attempts = 0;
};

class encryptor {
public:
	void set_key_ratchet(std::unique_ptr<key_ratchet_interface> keyRatchet);
	void set_passthrough_mode(bool passthroughMode);

	bool has_key_ratchet() const { return keyRatchet_ != nullptr; }
	bool is_passthrough_mode() const { return passthroughMode_; }

	void assign_ssrc_to_codec(uint32_t ssrc, codec codecType);
	codec codec_for_ssrc(uint32_t ssrc);

	int encrypt(media_type mediaType,
		uint32_t ssrc,
		array_view<const uint8_t> frame,
		array_view<uint8_t> encryptedFrame,
		size_t* bytesWritten);

	size_t get_max_ciphertext_byte_size(media_type mediaType, size_t frameSize);
	encryption_stats get_stats(media_type mediaType) const { return stats_[mediaType]; }

	using protocol_version_changed_callback = std::function<void()>;
	void set_protocol_version_changed_callback(protocol_version_changed_callback callback)
	{
		protocolVersionChangedCallback_ = std::move(callback);
	}
	protocol_version get_protocol_version() const { return currentProtocolVersion_; }

	enum result_code : uint8_t {
		rc_success,
		rc_encryption_failure,
	};

private:
	std::unique_ptr<outbound_frame_processor> get_or_create_frame_processor();
	void return_frame_processor(std::unique_ptr<outbound_frame_processor> frameProcessor);

	using cryptor_and_nonce = std::pair<std::shared_ptr<cipher_interface>, truncated_sync_nonce>;
	cryptor_and_nonce get_next_cryptor_and_nonce();

	void update_current_protocol_version(protocol_version version);

	std::atomic_bool passthroughMode_{false};

	std::mutex keyGenMutex_;
	std::unique_ptr<key_ratchet_interface> keyRatchet_;
	std::shared_ptr<cipher_interface> cryptor_;
	key_generation currentKeyGeneration_{0};
	truncated_sync_nonce truncatedNonce_{0};

	std::mutex frameProcessorsMutex_;
	std::vector<std::unique_ptr<outbound_frame_processor>> frameProcessors_;

	using SsrcCodecPair = std::pair<uint32_t, codec>;
	std::vector<SsrcCodecPair> ssrcCodecPairs_;

	using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
	TimePoint lastStatsTime_{TimePoint::min()};
	std::array<encryption_stats, 2> stats_;

	protocol_version_changed_callback protocolVersionChangedCallback_;
	protocol_version currentProtocolVersion_{max_protocol_version()};
};

} // namespace dpp::dave

