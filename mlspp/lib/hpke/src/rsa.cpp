#include "rsa.h"

#include "common.h"
#include "openssl/rsa.h"
#include "openssl_common.h"

namespace mlspp::hpke {

std::unique_ptr<Signature::PrivateKey>
RSASignature::generate_key_pair() const
{
  throw std::runtime_error("Not implemented");
}

std::unique_ptr<Signature::PrivateKey>
RSASignature::derive_key_pair(const bytes& /*ikm*/) const
{
  throw std::runtime_error("Not implemented");
}

std::unique_ptr<Signature::PrivateKey>
RSASignature::generate_key_pair(size_t bits)
{
  auto ctx = make_typed_unique(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
  if (ctx == nullptr) {
    throw openssl_error();
  }

  if (EVP_PKEY_keygen_init(ctx.get()) <= 0) {
    throw openssl_error();
  }

  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), static_cast<int>(bits)) <=
      0) {
    throw openssl_error();
  }

  auto* pkey = static_cast<EVP_PKEY*>(nullptr);
  if (EVP_PKEY_keygen(ctx.get(), &pkey) <= 0) {
    throw openssl_error();
  }

  return std::make_unique<PrivateKey>(pkey);
}

// TODO(rlb): Implement derive() with sizes

bytes
RSASignature::serialize(const Signature::PublicKey& pk) const
{
  const auto& rpk = dynamic_cast<const PublicKey&>(pk);
  const int len = i2d_PublicKey(rpk.pkey.get(), nullptr);
  auto raw = bytes(len);
  auto* data_ptr = raw.data();
  if (len != i2d_PublicKey(rpk.pkey.get(), &data_ptr)) {
    throw openssl_error();
  }
  return raw;
}

std::unique_ptr<Signature::PublicKey>
RSASignature::deserialize(const bytes& enc) const
{
  const auto* data_ptr = enc.data();
  auto* pkey = d2i_PublicKey(
    EVP_PKEY_RSA, nullptr, &data_ptr, static_cast<int>(enc.size()));
  if (pkey == nullptr) {
    throw openssl_error();
  }
  return std::make_unique<RSASignature::PublicKey>(pkey);
}

bytes
RSASignature::serialize_private(const Signature::PrivateKey& sk) const
{
  const auto& rsk = dynamic_cast<const PrivateKey&>(sk);
  const int len = i2d_PrivateKey(rsk.pkey.get(), nullptr);
  auto raw = bytes(len);
  auto* data_ptr = raw.data();
  if (len != i2d_PrivateKey(rsk.pkey.get(), &data_ptr)) {
    throw openssl_error();
  }

  return raw;
}

std::unique_ptr<Signature::PrivateKey>
RSASignature::deserialize_private(const bytes& skm) const
{
  const auto* data_ptr = skm.data();
  auto* pkey = d2i_PrivateKey(
    EVP_PKEY_RSA, nullptr, &data_ptr, static_cast<int>(skm.size()));
  if (pkey == nullptr) {
    throw openssl_error();
  }
  return std::make_unique<RSASignature::PrivateKey>(pkey);
}

bytes
RSASignature::sign(const bytes& data, const Signature::PrivateKey& sk) const
{
  const auto& rsk = dynamic_cast<const PrivateKey&>(sk);

  auto ctx = make_typed_unique(EVP_MD_CTX_create());
  if (ctx == nullptr) {
    throw openssl_error();
  }

  if (1 !=
      EVP_DigestSignInit(ctx.get(), nullptr, md, nullptr, rsk.pkey.get())) {
    throw openssl_error();
  }

  size_t siglen = EVP_PKEY_size(rsk.pkey.get());
  bytes sig(siglen);
  if (1 != EVP_DigestSign(
             ctx.get(), sig.data(), &siglen, data.data(), data.size())) {
    throw openssl_error();
  }

  sig.resize(siglen);
  return sig;
}

bool
RSASignature::verify(const bytes& data,
                     const bytes& sig,
                     const Signature::PublicKey& pk) const
{
  const auto& rpk = dynamic_cast<const PublicKey&>(pk);

  auto ctx = make_typed_unique(EVP_MD_CTX_create());
  if (ctx == nullptr) {
    throw openssl_error();
  }

  if (1 !=
      EVP_DigestVerifyInit(ctx.get(), nullptr, md, nullptr, rpk.pkey.get())) {
    throw openssl_error();
  }

  auto rv = EVP_DigestVerify(
    ctx.get(), sig.data(), sig.size(), data.data(), data.size());

  return rv == 1;
}

// TODO(RLB) Implement these methods.  No concrete need, but might be nice for
// completeness.
std::unique_ptr<Signature::PrivateKey>
RSASignature::import_jwk_private(const std::string& /* json_str */) const
{
  throw std::runtime_error("not implemented");
}

std::unique_ptr<Signature::PublicKey>
RSASignature::import_jwk(const std::string& /* json_str */) const
{
  throw std::runtime_error("not implemented");
}

std::string
RSASignature::export_jwk_private(const Signature::PrivateKey& /* sk */) const
{
  throw std::runtime_error("not implemented");
}

std::string
RSASignature::export_jwk(const Signature::PublicKey& /* pk */) const
{
  throw std::runtime_error("not implemented");
}

const EVP_MD*
RSASignature::digest_to_md(Digest::ID digest)
{
  // NOLINTNEXTLINE(hicpp-multiway-paths-covered)
  switch (digest) {
    case Digest::ID::SHA256:
      return EVP_sha256();
    case Digest::ID::SHA384:
      return EVP_sha384();
    case Digest::ID::SHA512:
      return EVP_sha512();
    default:
      throw std::runtime_error("Unsupported digest");
  }
}

Signature::ID
RSASignature::digest_to_sig(Digest::ID digest)
{
  // NOLINTNEXTLINE(hicpp-multiway-paths-covered)
  switch (digest) {
    case Digest::ID::SHA256:
      return Signature::ID::RSA_SHA256;
    case Digest::ID::SHA384:
      return Signature::ID::RSA_SHA384;
    case Digest::ID::SHA512:
      return Signature::ID::RSA_SHA512;
    default:
      throw std::runtime_error("Unsupported digest");
  }
}

} // namespace mlspp::hpke
