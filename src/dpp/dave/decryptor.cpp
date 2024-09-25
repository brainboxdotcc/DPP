#include "decryptor.h"

#include <bytes/bytes.h>

#include <cstring>
#include "common.h"
#include "logger.h"
#include "leb128.h"
#include "scope_exit.h"

using namespace std::chrono_literals;

namespace discord {
namespace dave {

constexpr auto kStatsInterval = 10s;

void Decryptor::TransitionToKeyRatchet(std::unique_ptr<IKeyRatchet> keyRatchet,
                                       Duration transitionExpiry)
{
    DISCORD_LOG(LS_INFO) << "Transitioning to new key ratchet: " << keyRatchet.get()
                         << ", expiry: " << transitionExpiry.count();

    // Update the expiry time for all existing cryptor managers
    UpdateCryptorManagerExpiry(transitionExpiry);

    if (keyRatchet) {
        cryptorManagers_.emplace_back(clock_, std::move(keyRatchet));
    }
}

void Decryptor::TransitionToPassthroughMode(bool passthroughMode, Duration transitionExpiry)
{
    if (passthroughMode) {
        allowPassThroughUntil_ = TimePoint::max();
    }
    else {
        // Update the pass through mode expiry
        auto maxExpiry = clock_.Now() + transitionExpiry;
        allowPassThroughUntil_ = std::min(allowPassThroughUntil_, maxExpiry);
    }
}

size_t Decryptor::Decrypt(MediaType mediaType,
                          ArrayView<const uint8_t> encryptedFrame,
                          ArrayView<uint8_t> frame)
{
    if (mediaType != Audio && mediaType != Video) {
        DISCORD_LOG(LS_WARNING) << "Decrypt failed, invalid media type: "
                                << static_cast<int>(mediaType);
        return 0;
    }

    auto start = clock_.Now();

    auto localFrame = GetOrCreateFrameProcessor();
    ScopeExit cleanup([&] { ReturnFrameProcessor(std::move(localFrame)); });

    // Skip decrypting for silence frames
    if (mediaType == Audio && encryptedFrame.size() == kOpusSilencePacket.size() &&
	std::memcmp(encryptedFrame.data(), kOpusSilencePacket.data(), kOpusSilencePacket.size()) == 0) {
        DISCORD_LOG(LS_VERBOSE) << "Decrypt skipping silence of size: " << encryptedFrame.size();
        if (encryptedFrame.data() != frame.data()) {
		std::memcpy(frame.data(), encryptedFrame.data(), encryptedFrame.size());
        }
        return encryptedFrame.size();
    }

    // Remove any expired cryptor manager
    CleanupExpiredCryptorManagers();

    // Process the incoming frame
    // This will check whether it looks like a valid encrypted frame
    // and if so it will parse it into its different components
    localFrame->ParseFrame(encryptedFrame);

    // If the frame is not encrypted and we can pass it through, do it
    bool canUsePassThrough = allowPassThroughUntil_ > start;
    if (!localFrame->IsEncrypted() && canUsePassThrough) {
        if (encryptedFrame.data() != frame.data()) {
		std::memcpy(frame.data(), encryptedFrame.data(), encryptedFrame.size());
        }
        stats_[mediaType].passthroughCount++;
        return encryptedFrame.size();
    }

    // If the frame is not encrypted and we can't pass it through, fail
    if (!localFrame->IsEncrypted()) {
        DISCORD_LOG(LS_INFO)
          << "Decrypt failed, frame is not encrypted and pass through is disabled";
        stats_[mediaType].decryptFailureCount++;
        return 0;
    }

    // Try and decrypt with each valid cryptor
    // reverse iterate to try the newest cryptors first
    bool success = false;
    for (auto it = cryptorManagers_.rbegin(); it != cryptorManagers_.rend(); ++it) {
        auto& cryptorManager = *it;
        success = DecryptImpl(cryptorManager, mediaType, *localFrame, frame);
        if (success) {
            break;
        }
    }

    size_t bytesWritten = 0;
    if (success) {
        stats_[mediaType].decryptSuccessCount++;
        bytesWritten = localFrame->ReconstructFrame(frame);
    }
    else {
        stats_[mediaType].decryptFailureCount++;
        DISCORD_LOG(LS_WARNING) << "Decrypt failed, no valid cryptor found, type: "
                                << (mediaType ? "video" : "audio")
                                << ", encrypted frame size: " << encryptedFrame.size()
                                << ", plaintext frame size: " << frame.size()
                                << ", number of cryptor managers: " << cryptorManagers_.size()
                                << ", pass through enabled: " << (canUsePassThrough ? "yes" : "no");
    }

    auto end = clock_.Now();
    if (end > lastStatsTime_ + kStatsInterval) {
        lastStatsTime_ = end;
        DISCORD_LOG(LS_INFO) << "Decrypted audio: " << stats_[Audio].decryptSuccessCount
                             << ", video: " << stats_[Video].decryptSuccessCount
                             << ". Failed audio: " << stats_[Audio].decryptFailureCount
                             << ", video: " << stats_[Video].decryptFailureCount;
    }
    stats_[mediaType].decryptDuration +=
      std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    return bytesWritten;
}

bool Decryptor::DecryptImpl(CryptorManager& cryptorManager,
                            MediaType mediaType,
                            InboundFrameProcessor& encryptedFrame,
                            ArrayView<uint8_t> frame)
{
    auto tag = encryptedFrame.GetTag();
    auto truncatedNonce = encryptedFrame.GetTruncatedNonce();

    auto authenticatedData = encryptedFrame.GetAuthenticatedData();
    auto ciphertext = encryptedFrame.GetCiphertext();
    auto plaintext = encryptedFrame.GetPlaintext();

    // expand the truncated nonce to the full sized one needed for decryption
    auto nonceBuffer = std::array<uint8_t, kAesGcm128NonceBytes>();
    memcpy(nonceBuffer.data() + kAesGcm128TruncatedSyncNonceOffset,
           &truncatedNonce,
           kAesGcm128TruncatedSyncNonceBytes);

    auto nonceBufferView = MakeArrayView<const uint8_t>(nonceBuffer.data(), nonceBuffer.size());

    auto generation =
      cryptorManager.ComputeWrappedGeneration(truncatedNonce >> kRatchetGenerationShiftBits);

    if (!cryptorManager.CanProcessNonce(generation, truncatedNonce)) {
        DISCORD_LOG(LS_INFO) << "Decrypt failed, cannot process nonce: " << truncatedNonce;
        return false;
    }

    // Get the cryptor for this generation
    ICryptor* cryptor = cryptorManager.GetCryptor(generation);

    if (!cryptor) {
        DISCORD_LOG(LS_INFO) << "Decrypt failed, no cryptor found for generation: " << generation;
        return false;
    }

    // perform the decryption
    bool success = cryptor->Decrypt(plaintext, ciphertext, tag, nonceBufferView, authenticatedData);
    stats_[mediaType].decryptAttempts++;

    if (success) {
        cryptorManager.ReportCryptorSuccess(generation, truncatedNonce);
    }

    return success;
}

size_t Decryptor::GetMaxPlaintextByteSize(MediaType mediaType, size_t encryptedFrameSize)
{
    return encryptedFrameSize;
}

void Decryptor::UpdateCryptorManagerExpiry(Duration expiry)
{
    auto maxExpiryTime = clock_.Now() + expiry;
    for (auto& cryptorManager : cryptorManagers_) {
        cryptorManager.UpdateExpiry(maxExpiryTime);
    }
}

void Decryptor::CleanupExpiredCryptorManagers()
{
    while (!cryptorManagers_.empty() && cryptorManagers_.front().IsExpired()) {
        DISCORD_LOG(LS_INFO) << "Removing expired cryptor manager.";
        cryptorManagers_.pop_front();
    }
}

std::unique_ptr<InboundFrameProcessor> Decryptor::GetOrCreateFrameProcessor()
{
    std::lock_guard<std::mutex> lock(frameProcessorsMutex_);
    if (frameProcessors_.empty()) {
        return std::make_unique<InboundFrameProcessor>();
    }
    auto frameProcessor = std::move(frameProcessors_.back());
    frameProcessors_.pop_back();
    return frameProcessor;
}

void Decryptor::ReturnFrameProcessor(std::unique_ptr<InboundFrameProcessor> frameProcessor)
{
    std::lock_guard<std::mutex> lock(frameProcessorsMutex_);
    frameProcessors_.push_back(std::move(frameProcessor));
}

} // namespace dave
} // namespace discord
