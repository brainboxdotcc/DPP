#pragma once

#include <mls/key_schedule.h>
#include "key_ratchet.h"

namespace discord {
namespace dave {

class MlsKeyRatchet : public IKeyRatchet {
public:
    MlsKeyRatchet(::mlspp::CipherSuite suite, bytes baseSecret) noexcept;
    ~MlsKeyRatchet() noexcept override;

    EncryptionKey GetKey(KeyGeneration generation) noexcept override;
    void DeleteKey(KeyGeneration generation) noexcept override;

private:
    ::mlspp::HashRatchet hashRatchet_;
};

} // namespace dave
} // namespace discord
