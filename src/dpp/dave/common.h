#pragma once

#include <array>
#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "version.h"

namespace mlspp::bytes_ns {
struct bytes;
};

namespace discord {
namespace dave {

using UnencryptedFrameHeaderSize = uint16_t;
using TruncatedSyncNonce = uint32_t;
using MagicMarker = uint16_t;
using EncryptionKey = ::mlspp::bytes_ns::bytes;
using TransitionId = uint16_t;
using SupplementalBytesSize = uint8_t;

enum MediaType : uint8_t { Audio, Video };
enum Codec : uint8_t { Unknown, Opus, VP8, VP9, H264, H265, AV1 };

// Returned in std::variant when a message is hard-rejected and should trigger a reset
struct failed_t {};

// Returned in std::variant when a message is soft-rejected and should not trigger a reset
struct ignored_t {};

// Map of ID-key pairs.
// In ProcessCommit, this lists IDs whose keys have been added, changed, or removed;
// an empty value value means a key was removed.
using RosterMap = std::map<uint64_t, std::vector<uint8_t>>;

// Return type for functions producing RosterMap or hard or soft failures
using RosterVariant = std::variant<failed_t, ignored_t, RosterMap>;

constexpr MagicMarker kMarkerBytes = 0xFAFA;

// Layout constants
constexpr size_t kAesGcm128KeyBytes = 16;
constexpr size_t kAesGcm128NonceBytes = 12;
constexpr size_t kAesGcm128TruncatedSyncNonceBytes = 4;
constexpr size_t kAesGcm128TruncatedSyncNonceOffset =
  kAesGcm128NonceBytes - kAesGcm128TruncatedSyncNonceBytes;
constexpr size_t kAesGcm128TruncatedTagBytes = 8;
constexpr size_t kRatchetGenerationBytes = 1;
constexpr size_t kRatchetGenerationShiftBits =
  8 * (kAesGcm128TruncatedSyncNonceBytes - kRatchetGenerationBytes);
constexpr size_t kSupplementalBytes =
  kAesGcm128TruncatedTagBytes + sizeof(SupplementalBytesSize) + sizeof(MagicMarker);
constexpr size_t kTransformPaddingBytes = 64;

// Timing constants
constexpr auto kDefaultTransitionDuration = std::chrono::seconds(10);
constexpr auto kCryptorExpiry = std::chrono::seconds(10);

// Behavior constants
constexpr auto kInitTransitionId = 0;
constexpr auto kDisabledVersion = 0;
constexpr auto kMaxGenerationGap = 250;
constexpr auto kMaxMissingNonces = 1000;
constexpr auto kGenerationWrap = 1 << (8 * kRatchetGenerationBytes);
constexpr auto kMaxFramesPerSecond = 50 + 2 * 60; // 50 audio frames + 2 * 60fps video streams
constexpr std::array<uint8_t, 3> kOpusSilencePacket = {0xF8, 0xFF, 0xFE};

// Utility routine for variant return types
template <class T, class V>
inline std::optional<T> GetOptional(V&& variant)
{
    if (auto map = std::get_if<T>(&variant)) {
        if constexpr (std::is_rvalue_reference_v<decltype(variant)>) {
            return std::move(*map);
        }
        else {
            return *map;
        }
    }
    else {
        return std::nullopt;
    }
}

} // namespace dave
} // namespace discord
