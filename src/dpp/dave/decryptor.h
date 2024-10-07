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
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "codec_utils.h"
#include "common.h"
#include "cipher_interface.h"
#include "cryptor_manager.h"
#include "frame_processors.h"
#include "version.h"
#include "clock.h"

namespace dpp {
	class cluster;
}

namespace dpp::dave {

class key_ratchet_interface;

/**
 * @brief Decryption stats
 */
struct decryption_stats {
	/**
	 * @brief Number of passthroughs
	 */
	uint64_t passthroughs = 0;
	/**
	 * @brief Number of decryption successes
	 */
	uint64_t decrypt_success = 0;
	/**
	 * @brief Number of decryption failures
	 */
	uint64_t decrypt_failure = 0;
	/**
	 * @brief Total encryption duration
	 */
	uint64_t decrypt_duration = 0;
	/**
	 * @brief Number of decryption attempts
	 */
	uint64_t decrypt_attempts = 0;
};

/**
 * @brief Decryptor, decrypts encrypted frames
 */
class decryptor {
public:
	/**
	 * @brief Constructor
	 * @param cl Creator
	 */
	decryptor(dpp::cluster& cl) : creator(cl) { };

	/**
	 * @brief Chrono duration
	 */
	using duration = std::chrono::seconds;

	/**
	 * @brief Set a new key ratchet for a decryptor. These are derived during welcome/commit
	 * of the session. Once you have a key ratchet, you can derive the key, and decrypt that
	 * user's audio/video.
	 *
	 * @param keyRatchet Key ratchet
	 * @param transitionExpiry Transition expiry. Old keys last this long before being withdrawn
	 * in preference of this new one.
	 */
	void transition_to_key_ratchet(std::unique_ptr<key_ratchet_interface> keyRatchet,
				       duration transitionExpiry = DEFAULT_TRANSITION_EXPIRY);

	/**
	 * @brief Transition to passthrough mode
	 *
	 * Passthrough mode occurs when a non-DAVE user connects to the VC.
	 *
	 * @param passthroughMode True to enable passthrough mode
	 * @param transitionExpiry Expiry for the transition
	 */
	void transition_to_passthrough_mode(bool passthroughMode,
					    duration transitionExpiry = DEFAULT_TRANSITION_EXPIRY);

	/**
	 * @brief Decrypt a frame
	 *
	 * @param mediaType type of media, audio or video
	 * @param encryptedFrame encrypted frame bytes
	 * @param frame plaintext output
	 * @return size of decrypted frame, or 0 if failure
	 */
	size_t decrypt(media_type mediaType,
		   array_view<const uint8_t> encryptedFrame,
		   array_view<uint8_t> frame);

	/**
	 * @brief Get maximum possible decrypted size of frame from an encrypted frame
	 * @param mediaType type of media
	 * @param encryptedFrameSize encrypted frame size
	 * @return size of plaintext buffer required
	 */
	size_t get_max_plaintext_byte_size(media_type mediaType, size_t encryptedFrameSize);

	/**
	 * @brief Get decryption stats
	 * @param mediaType media type, audio or video
	 * @return decryption stats
	 */
	decryption_stats get_stats(media_type mediaType) const { return stats_[mediaType]; }

private:
	/**
	 * @brief Chrono time point
	 */
	using time_point = clock_interface::time_point;

	/**
	 * @brief Decryption implementation
	 *
	 * @param cipher_manager cipher manager
	 * @param mediaType  media time, audio or video
	 * @param encryptedFrame  encrypted frame data
	 * @param frame decrypted frame data
	 * @return True if decryption succeeded
	 */
	bool decrypt_impl(aead_cipher_manager& cipher_manager, media_type mediaType, inbound_frame_processor& encryptedFrame, array_view<uint8_t> frame);

	/**
	 * @brief Update expiry for an instance of the manager
	 * @param expiry expiry duration
	 */
	void update_cryptor_manager_expiry(duration expiry);

	/**
	 * @brief Clean up expired cryptor managers
	 */
	void cleanup_expired_cryptor_managers();

	/**
	 * @brief Get frame procesor, or create a new one
	 * @return frame processor
	 */
	std::unique_ptr<inbound_frame_processor> get_or_create_frame_processor();

	/**
	 * Return frame processor
	 * @param frameProcessor frame processor
	 */
	void return_frame_processor(std::unique_ptr<inbound_frame_processor> frameProcessor);

	clock clock_;
	std::deque<aead_cipher_manager> cryptorManagers_;

	std::mutex frameProcessorsMutex_;
	std::vector<std::unique_ptr<inbound_frame_processor>> frameProcessors_;

	time_point allowPassThroughUntil_{time_point::min()};

	time_point lastStatsTime_{time_point::min()};
	std::array<decryption_stats, 2> stats_;

	/**
	 * @brief DPP Cluster, used for logging
	 */
	dpp::cluster& creator;
};

} // namespace dpp::dave

