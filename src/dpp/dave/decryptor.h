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

struct DecryptorStats {
    uint64_t passthroughCount = 0;
    uint64_t decryptSuccessCount = 0;
    uint64_t decryptFailureCount = 0;
    uint64_t decryptDuration = 0;
    uint64_t decryptAttempts = 0;
};

class Decryptor {
public:
    using Duration = std::chrono::seconds;

    void TransitionToKeyRatchet(std::unique_ptr<IKeyRatchet> keyRatchet,
                                Duration transitionExpiry = kDefaultTransitionDuration);
    void TransitionToPassthroughMode(bool passthroughMode,
                                     Duration transitionExpiry = kDefaultTransitionDuration);

    size_t decrypt(MediaType mediaType,
		   array_view<const uint8_t> encryptedFrame,
		   array_view<uint8_t> frame);

    size_t GetMaxPlaintextByteSize(MediaType mediaType, size_t encryptedFrameSize);
    DecryptorStats GetStats(MediaType mediaType) const { return stats_[mediaType]; }

private:
    using TimePoint = clock_interface::time_point;

    bool DecryptImpl(aead_cipher_manager& cryptor,
		     MediaType mediaType,
		     InboundFrameProcessor& encryptedFrame,
		     array_view<uint8_t> frame);

    void UpdateCryptorManagerExpiry(Duration expiry);
    void CleanupExpiredCryptorManagers();

    std::unique_ptr<InboundFrameProcessor> GetOrCreateFrameProcessor();
    void ReturnFrameProcessor(std::unique_ptr<InboundFrameProcessor> frameProcessor);

    Clock clock_;
    std::deque<aead_cipher_manager> cryptorManagers_;

    std::mutex frameProcessorsMutex_;
    std::vector<std::unique_ptr<InboundFrameProcessor>> frameProcessors_;

    TimePoint allowPassThroughUntil_{TimePoint::min()};

    TimePoint lastStatsTime_{TimePoint::min()};
    std::array<DecryptorStats, 2> stats_;
};

} // namespace dpp::dave

