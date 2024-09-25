#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>

#include "cryptor.h"
#include "key_ratchet.h"
#include "common.h"
#include "clock.h"

namespace discord {
namespace dave {

KeyGeneration ComputeWrappedGeneration(KeyGeneration oldest, KeyGeneration generation);

using BigNonce = uint64_t;
BigNonce ComputeWrappedBigNonce(KeyGeneration generation, TruncatedSyncNonce nonce);

class CryptorManager {
public:
    using TimePoint = typename IClock::TimePoint;

    CryptorManager(const IClock& clock, std::unique_ptr<IKeyRatchet> keyRatchet);

    void UpdateExpiry(TimePoint expiry) { ratchetExpiry_ = expiry; }
    bool IsExpired() const { return clock_.Now() > ratchetExpiry_; }

    bool CanProcessNonce(KeyGeneration generation, TruncatedSyncNonce nonce) const;
    KeyGeneration ComputeWrappedGeneration(KeyGeneration generation);

    ICryptor* GetCryptor(KeyGeneration generation);
    void ReportCryptorSuccess(KeyGeneration generation, TruncatedSyncNonce nonce);

private:
    struct ExpiringCryptor {
        std::unique_ptr<ICryptor> cryptor;
        TimePoint expiry;
    };

    ExpiringCryptor MakeExpiringCryptor(KeyGeneration generation);
    void CleanupExpiredCryptors();

    const IClock& clock_;
    std::unique_ptr<IKeyRatchet> keyRatchet_;
    std::unordered_map<KeyGeneration, ExpiringCryptor> cryptors_;

    TimePoint ratchetCreation_;
    TimePoint ratchetExpiry_;
    KeyGeneration oldestGeneration_{0};
    KeyGeneration newestGeneration_{0};

    std::optional<BigNonce> newestProcessedNonce_;
    std::deque<BigNonce> missingNonces_;
};

} // namespace dave
} // namespace discord
