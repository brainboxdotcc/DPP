#include "cryptor_manager.h"

#include <limits>

#include "key_ratchet.h"
#include "logger.h"

#include <bytes/bytes.h>

using namespace std::chrono_literals;

namespace discord {
namespace dave {

KeyGeneration ComputeWrappedGeneration(KeyGeneration oldest, KeyGeneration generation)
{
    // Assume generation is greater than or equal to oldest, this may be wrong in a few cases but
    // will be caught by the max generation gap check.
    auto remainder = oldest % kGenerationWrap;
    auto factor = oldest / kGenerationWrap + (generation < remainder ? 1 : 0);
    return factor * kGenerationWrap + generation;
}

BigNonce ComputeWrappedBigNonce(KeyGeneration generation, TruncatedSyncNonce nonce)
{
    // Remove the generation bits from the nonce
    auto maskedNonce = nonce & ((1 << kRatchetGenerationShiftBits) - 1);
    // Add the wrapped generation bits back in
    return static_cast<BigNonce>(generation) << kRatchetGenerationShiftBits | maskedNonce;
}

CryptorManager::CryptorManager(const IClock& clock, std::unique_ptr<IKeyRatchet> keyRatchet)
  : clock_(clock)
  , keyRatchet_(std::move(keyRatchet))
  , ratchetCreation_(clock.Now())
  , ratchetExpiry_(TimePoint::max())
{
}

bool CryptorManager::CanProcessNonce(KeyGeneration generation, TruncatedSyncNonce nonce) const
{
    if (!newestProcessedNonce_) {
        return true;
    }

    auto bigNonce = ComputeWrappedBigNonce(generation, nonce);
    return bigNonce > *newestProcessedNonce_ ||
      std::find(missingNonces_.rbegin(), missingNonces_.rend(), bigNonce) != missingNonces_.rend();
}

ICryptor* CryptorManager::GetCryptor(KeyGeneration generation)
{
    CleanupExpiredCryptors();

    if (generation < oldestGeneration_) {
        DISCORD_LOG(LS_INFO) << "Received frame with old generation: " << generation
                             << ", oldest generation: " << oldestGeneration_;
        return nullptr;
    }

    if (generation > newestGeneration_ + kMaxGenerationGap) {
        DISCORD_LOG(LS_INFO) << "Received frame with future generation: " << generation
                             << ", newest generation: " << newestGeneration_;
        return nullptr;
    }

    auto ratchetLifetimeSec =
      std::chrono::duration_cast<std::chrono::seconds>(clock_.Now() - ratchetCreation_).count();
    auto maxLifetimeFrames = kMaxFramesPerSecond * ratchetLifetimeSec;
    auto maxLifetimeGenerations = maxLifetimeFrames >> kRatchetGenerationShiftBits;
    if (generation > maxLifetimeGenerations) {
        DISCORD_LOG(LS_INFO) << "Received frame with generation " << generation
                             << " beyond ratchet max lifetime generations: "
                             << maxLifetimeGenerations
                             << ", ratchet lifetime: " << ratchetLifetimeSec << "s";
        return nullptr;
    }

    auto it = cryptors_.find(generation);
    if (it == cryptors_.end()) {
        // We don't have a cryptor for this generation, create one
        std::tie(it, std::ignore) = cryptors_.emplace(generation, MakeExpiringCryptor(generation));
    }

    // Return a non-owning pointer to the cryptor
    auto& [cryptor, expiry] = it->second;
    return cryptor.get();
}

void CryptorManager::ReportCryptorSuccess(KeyGeneration generation, TruncatedSyncNonce nonce)
{
    auto bigNonce = ComputeWrappedBigNonce(generation, nonce);

    // Add any missing nonces to the queue
    if (!newestProcessedNonce_) {
        newestProcessedNonce_ = bigNonce;
    }
    else if (bigNonce > *newestProcessedNonce_) {
        auto oldestMissingNonce = bigNonce > kMaxMissingNonces ? bigNonce - kMaxMissingNonces : 0;

        while (!missingNonces_.empty() && missingNonces_.front() < oldestMissingNonce) {
            missingNonces_.pop_front();
        }

        // If we're missing a lot, we don't want to add everything since newestProcessedNonce_
        auto missingRangeStart = std::max(oldestMissingNonce, *newestProcessedNonce_ + 1);
        for (auto i = missingRangeStart; i < bigNonce; ++i) {
            missingNonces_.push_back(i);
        }

        // Update the newest processed nonce
        newestProcessedNonce_ = bigNonce;
    }
    else {
        auto it = std::find(missingNonces_.begin(), missingNonces_.end(), bigNonce);
        if (it != missingNonces_.end()) {
            missingNonces_.erase(it);
        }
    }

    if (generation <= newestGeneration_ || cryptors_.find(generation) == cryptors_.end()) {
        return;
    }
    DISCORD_LOG(LS_INFO) << "Reporting cryptor success, generation: " << generation;
    newestGeneration_ = generation;

    // Update the expiry time for all old cryptors
    const auto expiryTime = clock_.Now() + kCryptorExpiry;
    for (auto& [gen, cryptor] : cryptors_) {
        if (gen < newestGeneration_) {
            DISCORD_LOG(LS_INFO) << "Updating expiry for cryptor, generation: " << gen;
            cryptor.expiry = std::min(cryptor.expiry, expiryTime);
        }
    }
}

KeyGeneration CryptorManager::ComputeWrappedGeneration(KeyGeneration generation)
{
    return ::discord::dave::ComputeWrappedGeneration(oldestGeneration_, generation);
}

CryptorManager::ExpiringCryptor CryptorManager::MakeExpiringCryptor(KeyGeneration generation)
{
    // Get the new key from the ratchet
    auto encryptionKey = keyRatchet_->GetKey(generation);
    auto expiryTime = TimePoint::max();

    // If we got frames out of order, we might have to create a cryptor for an old generation
    // In that case, create it with a non-infinite expiry time as we have already transitioned
    // to a newer generation
    if (generation < newestGeneration_) {
        DISCORD_LOG(LS_INFO) << "Creating cryptor for old generation: " << generation;
        expiryTime = clock_.Now() + kCryptorExpiry;
    }
    else {
        DISCORD_LOG(LS_INFO) << "Creating cryptor for new generation: " << generation;
    }

    return {CreateCryptor(encryptionKey), expiryTime};
}

void CryptorManager::CleanupExpiredCryptors()
{
    for (auto it = cryptors_.begin(); it != cryptors_.end();) {
        auto& [generation, cryptor] = *it;

        bool expired = cryptor.expiry < clock_.Now();
        if (expired) {
            DISCORD_LOG(LS_INFO) << "Removing expired cryptor, generation: " << generation;
        }

        it = expired ? cryptors_.erase(it) : ++it;
    }

    while (oldestGeneration_ < newestGeneration_ &&
           cryptors_.find(oldestGeneration_) == cryptors_.end()) {
        DISCORD_LOG(LS_INFO) << "Deleting key for old generation: " << oldestGeneration_;
        keyRatchet_->DeleteKey(oldestGeneration_);
        ++oldestGeneration_;
    }
}

} // namespace dave
} // namespace discord
