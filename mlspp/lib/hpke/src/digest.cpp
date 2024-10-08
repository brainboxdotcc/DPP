#include <hpke/digest.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>
#if defined(WITH_OPENSSL3)
#include <openssl/core_names.h>
#endif

#include "openssl_common.h"

namespace mlspp::hpke {

static const EVP_MD*
openssl_digest_type(Digest::ID digest)
{
  switch (digest) {
    case Digest::ID::SHA256:
      return EVP_sha256();

    case Digest::ID::SHA384:
      return EVP_sha384();

    case Digest::ID::SHA512:
      return EVP_sha512();

    default:
      throw std::runtime_error("Unsupported ciphersuite");
  }
}

#if defined(WITH_OPENSSL3)
static std::string
openssl_digest_name(Digest::ID digest)
{
  switch (digest) {
    case Digest::ID::SHA256:
      return OSSL_DIGEST_NAME_SHA2_256;

    case Digest::ID::SHA384:
      return OSSL_DIGEST_NAME_SHA2_384;

    case Digest::ID::SHA512:
      return OSSL_DIGEST_NAME_SHA2_512;

    default:
      throw std::runtime_error("Unsupported digest algorithm");
  }
}
#endif

template<>
const Digest&
Digest::get<Digest::ID::SHA256>()
{
  static const Digest instance(Digest::ID::SHA256);
  return instance;
}

template<>
const Digest&
Digest::get<Digest::ID::SHA384>()
{
  static const Digest instance(Digest::ID::SHA384);
  return instance;
}

template<>
const Digest&
Digest::get<Digest::ID::SHA512>()
{
  static const Digest instance(Digest::ID::SHA512);
  return instance;
}

Digest::Digest(Digest::ID id_in)
  : id(id_in)
  , hash_size(EVP_MD_size(openssl_digest_type(id_in)))
{
}

bytes
Digest::hash(const bytes& data) const
{
  auto md = bytes(hash_size);
  unsigned int size = 0;
  const auto* type = openssl_digest_type(id);
  if (1 !=
      EVP_Digest(data.data(), data.size(), md.data(), &size, type, nullptr)) {
    throw openssl_error();
  }

  return md;
}

bytes
Digest::hmac(const bytes& key, const bytes& data) const
{
  auto md = bytes(hash_size);
  unsigned int size = 0;
  const auto* type = openssl_digest_type(id);
  if (nullptr == HMAC(type,
                      key.data(),
                      static_cast<int>(key.size()),
                      data.data(),
                      static_cast<int>(data.size()),
                      md.data(),
                      &size)) {
    throw openssl_error();
  }

  return md;
}

bytes
Digest::hmac_for_hkdf_extract(const bytes& key, const bytes& data) const
{
#if defined(WITH_OPENSSL3)
  auto digest_name = openssl_digest_name(id);
  std::array<OSSL_PARAM, 2> params = {
    OSSL_PARAM_construct_utf8_string(
      OSSL_ALG_PARAM_DIGEST, digest_name.data(), 0),
    OSSL_PARAM_construct_end()
  };
  const auto mac =
    make_typed_unique(EVP_MAC_fetch(nullptr, OSSL_MAC_NAME_HMAC, nullptr));
  const auto ctx = make_typed_unique(EVP_MAC_CTX_new(mac.get()));
#else
  const auto* type = openssl_digest_type(id);
  auto ctx = make_typed_unique(HMAC_CTX_new());
#endif
  if (ctx == nullptr) {
    throw openssl_error();
  }

  // Some FIPS-enabled libraries are overly conservative in their interpretation
  // of NIST SP 800-131A, which requires HMAC keys to be at least 112 bits long.
  // That document does not impose that requirement on HKDF, so we disable FIPS
  // enforcement for purposes of HKDF.
  //
  // https://doi.org/10.6028/NIST.SP.800-131Ar2
  auto key_size = static_cast<int>(key.size());
  // OpenSSL 3 does not support the flag EVP_MD_CTX_FLAG_NON_FIPS_ALLOW anymore.
  // However, OpenSSL 3 in FIPS mode doesn't seem to check the HMAC key size
  // constraint.
#if !defined(WITH_OPENSSL3) && !defined(WITH_BORINGSSL)
  static const auto fips_min_hmac_key_len = 14;
  if (FIPS_mode() != 0 && key_size < fips_min_hmac_key_len) {
    HMAC_CTX_set_flags(ctx.get(), EVP_MD_CTX_FLAG_NON_FIPS_ALLOW);
  }
#endif

  // Guard against sending nullptr to HMAC_Init_ex
  const auto* key_data = key.data();
  const auto non_null_zero_length_key = uint8_t(0);
  if (key_data == nullptr) {
    key_data = &non_null_zero_length_key;
  }

  auto md = bytes(hash_size);
#if defined(WITH_OPENSSL3)
  if (1 != EVP_MAC_init(ctx.get(), key_data, key_size, params.data())) {
    throw openssl_error();
  }
  if (1 != EVP_MAC_update(ctx.get(), data.data(), data.size())) {
    throw openssl_error();
  }
  size_t size = 0;
  if (1 != EVP_MAC_final(ctx.get(), md.data(), &size, hash_size)) {
    throw openssl_error();
  }
#else
  if (1 != HMAC_Init_ex(ctx.get(), key_data, key_size, type, nullptr)) {
    throw openssl_error();
  }
  if (1 != HMAC_Update(ctx.get(), data.data(), data.size())) {
    throw openssl_error();
  }
  unsigned int size = 0;
  if (1 != HMAC_Final(ctx.get(), md.data(), &size)) {
    throw openssl_error();
  }
#endif

  return md;
}

} // namespace mlspp::hpke
