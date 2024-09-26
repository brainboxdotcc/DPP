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

struct EncryptorStats {
    uint64_t passthroughCount = 0;
    uint64_t encryptSuccessCount = 0;
    uint64_t encryptFailureCount = 0;
    uint64_t encryptDuration = 0;
    uint64_t encryptAttempts = 0;
    uint64_t encryptMaxAttempts = 0;
};

class Encryptor {
public:
    void SetKeyRatchet(std::unique_ptr<IKeyRatchet> keyRatchet);
    void SetPassthroughMode(bool passthroughMode);

    bool HasKeyRatchet() const { return keyRatchet_ != nullptr; }
    bool IsPassthroughMode() const { return passthroughMode_; }

    void AssignSsrcToCodec(uint32_t ssrc, Codec codecType);
    Codec CodecForSsrc(uint32_t ssrc);

    int Encrypt(MediaType mediaType,
		uint32_t ssrc,
		array_view<const uint8_t> frame,
		array_view<uint8_t> encryptedFrame,
		size_t* bytesWritten);

    size_t GetMaxCiphertextByteSize(MediaType mediaType, size_t frameSize);
    EncryptorStats GetStats(MediaType mediaType) const { return stats_[mediaType]; }

    using ProtocolVersionChangedCallback = std::function<void()>;
    void SetProtocolVersionChangedCallback(ProtocolVersionChangedCallback callback)
    {
        protocolVersionChangedCallback_ = std::move(callback);
    }
    ProtocolVersion GetProtocolVersion() const { return currentProtocolVersion_; }

private:
    std::unique_ptr<OutboundFrameProcessor> GetOrCreateFrameProcessor();
    void ReturnFrameProcessor(std::unique_ptr<OutboundFrameProcessor> frameProcessor);

    using CryptorAndNonce = std::pair<std::shared_ptr<cipher_interface>, TruncatedSyncNonce>;
    CryptorAndNonce GetNextCryptorAndNonce();

    void UpdateCurrentProtocolVersion(ProtocolVersion version);

    enum ResultCode {
        Success,
        UninitializedContext,
        InitializationFailure,
        UnsupportedCodec,
        EncryptionFailure,
        FinalizationFailure,
        TagAppendFailure
    };

    std::atomic_bool passthroughMode_{false};

    std::mutex keyGenMutex_;
    std::unique_ptr<IKeyRatchet> keyRatchet_;
    std::shared_ptr<cipher_interface> cryptor_;
    KeyGeneration currentKeyGeneration_{0};
    TruncatedSyncNonce truncatedNonce_{0};

    std::mutex frameProcessorsMutex_;
    std::vector<std::unique_ptr<OutboundFrameProcessor>> frameProcessors_;

    using SsrcCodecPair = std::pair<uint32_t, Codec>;
    std::vector<SsrcCodecPair> ssrcCodecPairs_;

    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    TimePoint lastStatsTime_{TimePoint::min()};
    std::array<EncryptorStats, 2> stats_;

    ProtocolVersionChangedCallback protocolVersionChangedCallback_;
    ProtocolVersion currentProtocolVersion_{MaxSupportedProtocolVersion()};
};

} // namespace dpp::dave

