#pragma once

#include <memory>

#include "common.h"
#include "array_view.h"

namespace discord {
namespace dave {

class ICryptor {
public:
    virtual ~ICryptor() = default;

    virtual bool Encrypt(ArrayView<uint8_t> ciphertextBufferOut,
                         ArrayView<const uint8_t> plaintextBuffer,
                         ArrayView<const uint8_t> nonceBuffer,
                         ArrayView<const uint8_t> additionalData,
                         ArrayView<uint8_t> tagBufferOut) = 0;
    virtual bool Decrypt(ArrayView<uint8_t> plaintextBufferOut,
                         ArrayView<const uint8_t> ciphertextBuffer,
                         ArrayView<const uint8_t> tagBuffer,
                         ArrayView<const uint8_t> nonceBuffer,
                         ArrayView<const uint8_t> additionalData) = 0;
};

std::unique_ptr<ICryptor> CreateCryptor(const EncryptionKey& encryptionKey);

} // namespace dave
} // namespace discord
