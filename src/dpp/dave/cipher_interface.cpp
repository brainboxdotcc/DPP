#include "cipher_interface.h"
#include "openssl_aead_cipher.h"

namespace dpp::dave {

std::unique_ptr<cipher_interface> create_cipher(const EncryptionKey& encryptionKey)
{
    auto cipher = std::make_unique<openssl_aead_cipher>(encryptionKey);
    return cipher->IsValid() ? std::move(cipher) : nullptr;
}

} // namespace dpp::dave

