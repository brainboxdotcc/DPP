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

