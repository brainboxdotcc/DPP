#include "cryptor.h"
#include "openssl_cryptor.h"

namespace discord {
namespace dave {

std::unique_ptr<ICryptor> CreateCryptor(const EncryptionKey& encryptionKey)
{
    auto cryptor = std::make_unique<BoringSSLCryptor>(encryptionKey);
    return cryptor->IsValid() ? std::move(cryptor) : nullptr;
}

} // namespace dave
} // namespace discord
