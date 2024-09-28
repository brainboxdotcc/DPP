#pragma once

#include <memory>

#include "common.h"
#include "array_view.h"

namespace dpp::dave {

using const_byte_view = array_view<const uint8_t>;
using byte_view = array_view<uint8_t>;

class cipher_interface {
public:
	virtual ~cipher_interface() = default;

	cipher_interface() = default;
	cipher_interface(cipher_interface&&) = delete;
	cipher_interface(cipher_interface&) = delete;
	cipher_interface operator=(cipher_interface&&) = delete;
	cipher_interface operator=(cipher_interface&) = delete;

	virtual bool encrypt(byte_view ciphertextBufferOut, const_byte_view plaintextBuffer, const_byte_view nonceBuffer, const_byte_view additionalData, byte_view tagBufferOut) = 0;
	virtual bool decrypt(byte_view plaintextBufferOut, const_byte_view ciphertextBuffer, const_byte_view tagBuffer, const_byte_view nonceBuffer, const_byte_view additionalData) = 0;
};

std::unique_ptr<cipher_interface> create_cipher(const EncryptionKey& encryptionKey);

} // namespace dpp::dave

