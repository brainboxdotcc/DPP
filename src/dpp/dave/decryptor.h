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

namespace dpp::dave {

class IKeyRatchet;

struct decryption_stats {
    uint64_t passthroughs = 0;
    uint64_t decrypt_success = 0;
    uint64_t decrypt_failure = 0;
    uint64_t decrypt_duration = 0;
    uint64_t decrypt_attempts = 0;
};

class decryptor {
public:
    using Duration = std::chrono::seconds;

    void transition_to_key_ratchet(std::unique_ptr<IKeyRatchet> keyRatchet,
				   Duration transitionExpiry = kDefaultTransitionDuration);
    void transition_to_passthrough_mode(bool passthroughMode,
					Duration transitionExpiry = kDefaultTransitionDuration);

    size_t decrypt(media_type mediaType,
		   array_view<const uint8_t> encryptedFrame,
		   array_view<uint8_t> frame);

    size_t get_max_plaintext_byte_size(media_type mediaType, size_t encryptedFrameSize);
    decryption_stats get_stats(media_type mediaType) const { return stats_[mediaType]; }

private:
    using time_point = clock_interface::time_point;

    bool decrypt_impl(aead_cipher_manager& cipher_manager,
		      media_type mediaType,
		      inbound_frame_processor& encryptedFrame,
		      array_view<uint8_t> frame);

    void update_cryptor_manager_expiry(Duration expiry);
    void cleanup_expired_cryptor_managers();

    std::unique_ptr<inbound_frame_processor> get_or_create_frame_processor();
    void return_frame_processor(std::unique_ptr<inbound_frame_processor> frameProcessor);

    clock clock_;
    std::deque<aead_cipher_manager> cryptorManagers_;

    std::mutex frameProcessorsMutex_;
    std::vector<std::unique_ptr<inbound_frame_processor>> frameProcessors_;

    time_point allowPassThroughUntil_{time_point::min()};

    time_point lastStatsTime_{time_point::min()};
    std::array<decryption_stats, 2> stats_;
};

} // namespace dpp::dave

