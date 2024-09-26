#pragma once

#include <memory>

#include "common.h"
#include "array_view.h"

namespace dpp::dave {

class cipher_interface {
public:
    virtual ~cipher_interface() = default;

    virtual bool Encrypt(array_view<uint8_t> ciphertextBufferOut,
			 array_view<const uint8_t> plaintextBuffer,
			 array_view<const uint8_t> nonceBuffer,
			 array_view<const uint8_t> additionalData,
			 array_view<uint8_t> tagBufferOut) = 0;
    virtual bool Decrypt(array_view<uint8_t> plaintextBufferOut,
			 array_view<const uint8_t> ciphertextBuffer,
			 array_view<const uint8_t> tagBuffer,
			 array_view<const uint8_t> nonceBuffer,
			 array_view<const uint8_t> additionalData) = 0;
};

std::unique_ptr<cipher_interface> create_cipher(const EncryptionKey& encryptionKey);

} // namespace dpp::dave

