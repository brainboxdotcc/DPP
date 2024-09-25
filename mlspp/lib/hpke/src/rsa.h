#pragma once

#include <hpke/digest.h>
#include <hpke/hpke.h>
#include <hpke/signature.h>

#include "openssl_common.h"
#include <openssl/evp.h>
#include <openssl/rsa.h>

namespace mlspp::hpke {

// XXX(RLB): There is a lot of code in RSASignature that is duplicated in
// EVPGroup.  I have allowed this duplication rather than factoring it out
// because I would like to be able to cleanly remove RSA later.
struct RSASignature : public Signature
{
  struct PublicKey : public Signature::PublicKey
  {
    explicit PublicKey(EVP_PKEY* pkey_in)
      : pkey(pkey_in, typed_delete<EVP_PKEY>)
    {
    }

    ~PublicKey() override = default;

    typed_unique_ptr<EVP_PKEY> pkey;
  };

  struct PrivateKey : public Signature::PrivateKey
  {
    explicit PrivateKey(EVP_PKEY* pkey_in)
      : pkey(pkey_in, typed_delete<EVP_PKEY>)
    {
    }

    ~PrivateKey() override = default;

    std::unique_ptr<Signature::PublicKey> public_key() const override
    {
      if (1 != EVP_PKEY_up_ref(pkey.get())) {
        throw openssl_error();
      }
      return std::make_unique<PublicKey>(pkey.get());
    }

    typed_unique_ptr<EVP_PKEY> pkey;
  };

  explicit RSASignature(Digest::ID digest)
    : Signature(digest_to_sig(digest))
    , md(digest_to_md(digest))
  {
  }

  std::unique_ptr<Signature::PrivateKey> generate_key_pair() const override;

  std::unique_ptr<Signature::PrivateKey> derive_key_pair(
    const bytes& /*ikm*/) const override;

  static std::unique_ptr<Signature::PrivateKey> generate_key_pair(size_t bits);

  // TODO(rlb): Implement derive() with sizes

  bytes serialize(const Signature::PublicKey& pk) const override;

  std::unique_ptr<Signature::PublicKey> deserialize(
    const bytes& enc) const override;

  bytes serialize_private(const Signature::PrivateKey& sk) const override;

  std::unique_ptr<Signature::PrivateKey> deserialize_private(
    const bytes& skm) const override;

  bytes sign(const bytes& data, const Signature::PrivateKey& sk) const override;

  bool verify(const bytes& data,
              const bytes& sig,
              const Signature::PublicKey& pk) const override;

  std::unique_ptr<Signature::PrivateKey> import_jwk_private(
    const std::string& json_str) const override;
  std::unique_ptr<Signature::PublicKey> import_jwk(
    const std::string& json_str) const override;
  std::string export_jwk_private(
    const Signature::PrivateKey& sk) const override;
  std::string export_jwk(const Signature::PublicKey& pk) const override;

private:
  const EVP_MD* md;

  static const EVP_MD* digest_to_md(Digest::ID digest);

  static Signature::ID digest_to_sig(Digest::ID digest);
};

} // namespace mlspp::hpke
