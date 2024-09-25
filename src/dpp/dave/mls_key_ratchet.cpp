#include "mls_key_ratchet.h"

#include <cassert>

#include "logger.h"

namespace discord {
namespace dave {

MlsKeyRatchet::MlsKeyRatchet(::mlspp::CipherSuite suite, bytes baseSecret) noexcept
  : hashRatchet_(suite, std::move(baseSecret))
{
}

MlsKeyRatchet::~MlsKeyRatchet() noexcept = default;

EncryptionKey MlsKeyRatchet::GetKey(KeyGeneration generation) noexcept
{
    DISCORD_LOG(LS_INFO) << "Retrieving key for generation " << generation << " from HashRatchet";

    try {
        auto keyAndNonce = hashRatchet_.get(generation);
        assert(keyAndNonce.key.size() >= kAesGcm128KeyBytes);
        return std::move(keyAndNonce.key.as_vec());
    }
    catch (const std::exception& e) {
        DISCORD_LOG(LS_ERROR) << "Failed to retrieve key for generation " << generation << ": "
                              << e.what();
        return {};
    }
}

void MlsKeyRatchet::DeleteKey(KeyGeneration generation) noexcept
{
    hashRatchet_.erase(generation);
}

} // namespace dave
} // namespace discord
