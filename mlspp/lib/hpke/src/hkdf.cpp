#include "hkdf.h"
#include "openssl_common.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <stdexcept>

namespace mlspp::hpke {

template<>
const HKDF&
HKDF::get<Digest::ID::SHA256>()
{
  static const HKDF instance(Digest::get<Digest::ID::SHA256>());
  return instance;
}

template<>
const HKDF&
HKDF::get<Digest::ID::SHA384>()
{
  static const HKDF instance(Digest::get<Digest::ID::SHA384>());
  return instance;
}

template<>
const HKDF&
HKDF::get<Digest::ID::SHA512>()
{
  static const HKDF instance(Digest::get<Digest::ID::SHA512>());
  return instance;
}

static KDF::ID
digest_to_kdf(Digest::ID digest_id)
{
  switch (digest_id) {
    case Digest::ID::SHA256:
      return KDF::ID::HKDF_SHA256;
    case Digest::ID::SHA384:
      return KDF::ID::HKDF_SHA384;
    case Digest::ID::SHA512:
      return KDF::ID::HKDF_SHA512;
  }

  throw std::runtime_error("Unsupported algorithm");
}

HKDF::HKDF(const Digest& digest_in)
  : KDF(digest_to_kdf(digest_in.id), digest_in.hash_size)
  , digest(digest_in)
{
}

bytes
HKDF::extract(const bytes& salt, const bytes& ikm) const
{
  return digest.hmac_for_hkdf_extract(salt, ikm);
}

bytes
HKDF::expand(const bytes& prk, const bytes& info, size_t size) const
{
  auto okm = bytes{};
  auto i = uint8_t(0x00);
  auto Ti = bytes{};
  while (okm.size() < size) {
    i += 1;
    auto block = Ti + info + bytes{ i };

    Ti = digest.hmac(prk, block);
    okm += Ti;
  }

  okm.resize(size);
  return okm;
}

} // namespace mlspp::hpke
