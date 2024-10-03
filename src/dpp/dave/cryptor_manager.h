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

#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>

#include "cipher_interface.h"
#include "key_ratchet.h"
#include "common.h"
#include "clock.h"

namespace dpp::dave {

KeyGeneration compute_wrapped_generation(KeyGeneration oldest, KeyGeneration generation);

using BigNonce = uint64_t;
BigNonce compute_wrapped_big_nonce(KeyGeneration generation, TruncatedSyncNonce nonce);

class aead_cipher_manager {
public:
    using time_point = typename clock_interface::time_point;

    aead_cipher_manager(const clock_interface& clock, std::unique_ptr<IKeyRatchet> keyRatchet);

    void UpdateExpiry(time_point expiry) { ratchetExpiry_ = expiry; }
    bool IsExpired() const { return clock_.now() > ratchetExpiry_; }

    bool CanProcessNonce(KeyGeneration generation, TruncatedSyncNonce nonce) const;
    KeyGeneration ComputeWrappedGeneration(KeyGeneration generation);

    cipher_interface* GetCryptor(KeyGeneration generation);
    void ReportCryptorSuccess(KeyGeneration generation, TruncatedSyncNonce nonce);

private:
    struct ExpiringCryptor {
        std::unique_ptr<cipher_interface> cryptor;
        time_point expiry;
    };

    ExpiringCryptor MakeExpiringCryptor(KeyGeneration generation);
    void CleanupExpiredCryptors();

    const clock_interface& clock_;
    std::unique_ptr<IKeyRatchet> keyRatchet_;
    std::unordered_map<KeyGeneration, ExpiringCryptor> cryptors_;

    time_point ratchetCreation_;
    time_point ratchetExpiry_;
    KeyGeneration oldestGeneration_{0};
    KeyGeneration newestGeneration_{0};

    std::optional<BigNonce> newestProcessedNonce_;
    std::deque<BigNonce> missingNonces_;
};

} // namespace dpp::dave

