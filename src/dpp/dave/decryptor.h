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

    size_t Decrypt(MediaType mediaType,
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

