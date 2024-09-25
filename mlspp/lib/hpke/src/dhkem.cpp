#include "dhkem.h"

#include "common.h"

namespace mlspp::hpke {

DHKEM::PrivateKey::PrivateKey(Group::PrivateKey* group_priv_in)
  : group_priv(group_priv_in)
{
}

std::unique_ptr<KEM::PublicKey>
DHKEM::PrivateKey::public_key() const
{
  return group_priv->public_key();
}

DHKEM
make_dhkem(KEM::ID kem_id_in, const Group& group_in, const KDF& kdf_in)
{
  return { kem_id_in, group_in, kdf_in };
}

template<>
const DHKEM&
DHKEM::get<KEM::ID::DHKEM_P256_SHA256>()
{
  static const auto instance = make_dhkem(KEM::ID::DHKEM_P256_SHA256,
                                          Group::get<Group::ID::P256>(),
                                          KDF::get<KDF::ID::HKDF_SHA256>());
  return instance;
}

template<>
const DHKEM&
DHKEM::get<KEM::ID::DHKEM_P384_SHA384>()
{
  static const auto instance = make_dhkem(KEM::ID::DHKEM_P384_SHA384,
                                          Group::get<Group::ID::P384>(),
                                          KDF::get<KDF::ID::HKDF_SHA384>());
  return instance;
}

template<>
const DHKEM&
DHKEM::get<KEM::ID::DHKEM_P521_SHA512>()
{
  static const auto instance = make_dhkem(KEM::ID::DHKEM_P521_SHA512,
                                          Group::get<Group::ID::P521>(),
                                          KDF::get<KDF::ID::HKDF_SHA512>());
  return instance;
}

template<>
const DHKEM&
DHKEM::get<KEM::ID::DHKEM_X25519_SHA256>()
{
  static const auto instance = make_dhkem(KEM::ID::DHKEM_X25519_SHA256,
                                          Group::get<Group::ID::X25519>(),
                                          KDF::get<KDF::ID::HKDF_SHA256>());
  return instance;
}

#if !defined(WITH_BORINGSSL)
template<>
const DHKEM&
DHKEM::get<KEM::ID::DHKEM_X448_SHA512>()
{
  static const auto instance = make_dhkem(KEM::ID::DHKEM_X448_SHA512,
                                          Group::get<Group::ID::X448>(),
                                          KDF::get<KDF::ID::HKDF_SHA512>());
  return instance;
}
#endif

DHKEM::DHKEM(KEM::ID kem_id_in, const Group& group_in, const KDF& kdf_in)
  : KEM(kem_id_in,
        kdf_in.hash_size,
        group_in.pk_size,
        group_in.pk_size,
        group_in.sk_size)
  , group(group_in)
  , kdf(kdf_in)
{
  static const auto label_kem = from_ascii("KEM");
  suite_id = label_kem + i2osp(uint16_t(kem_id_in), 2);
}

std::unique_ptr<KEM::PrivateKey>
DHKEM::generate_key_pair() const
{
  return std::make_unique<DHKEM::PrivateKey>(
    group.generate_key_pair().release());
}

std::unique_ptr<KEM::PrivateKey>
DHKEM::derive_key_pair(const bytes& ikm) const
{
  return std::make_unique<DHKEM::PrivateKey>(
    group.derive_key_pair(suite_id, ikm).release());
}

bytes
DHKEM::serialize(const KEM::PublicKey& pk) const
{
  const auto& gpk = dynamic_cast<const Group::PublicKey&>(pk);
  return group.serialize(gpk);
}

std::unique_ptr<KEM::PublicKey>
DHKEM::deserialize(const bytes& enc) const
{
  return group.deserialize(enc);
}

bytes
DHKEM::serialize_private(const KEM::PrivateKey& sk) const
{
  const auto& gsk = dynamic_cast<const PrivateKey&>(sk);
  return group.serialize_private(*gsk.group_priv);
}

std::unique_ptr<KEM::PrivateKey>
DHKEM::deserialize_private(const bytes& skm) const
{
  return std::make_unique<PrivateKey>(group.deserialize_private(skm).release());
}

std::pair<bytes, bytes>
DHKEM::encap(const KEM::PublicKey& pkR) const
{
  const auto& gpkR = dynamic_cast<const Group::PublicKey&>(pkR);

  auto skE = group.generate_key_pair();
  auto pkE = skE->public_key();

  auto zz = group.dh(*skE, gpkR);
  auto enc = group.serialize(*pkE);

  auto pkRm = group.serialize(gpkR);
  auto kem_context = enc + pkRm;

  auto shared_secret = extract_and_expand(zz, kem_context);
  return std::make_pair(shared_secret, enc);
}

bytes
DHKEM::decap(const bytes& enc, const KEM::PrivateKey& skR) const
{
  const auto& gskR = dynamic_cast<const PrivateKey&>(skR);
  auto pkR = gskR.group_priv->public_key();
  auto pkE = group.deserialize(enc);
  auto zz = group.dh(*gskR.group_priv, *pkE);

  auto pkRm = group.serialize(*pkR);
  auto kem_context = enc + pkRm;
  return extract_and_expand(zz, kem_context);
}

std::pair<bytes, bytes>
DHKEM::auth_encap(const KEM::PublicKey& pkR, const KEM::PrivateKey& skS) const
{
  const auto& gpkR = dynamic_cast<const Group::PublicKey&>(pkR);
  const auto& gskS = dynamic_cast<const PrivateKey&>(skS);

  auto skE = group.generate_key_pair();
  auto pkE = skE->public_key();
  auto pkS = gskS.group_priv->public_key();

  auto zzER = group.dh(*skE, gpkR);
  auto zzSR = group.dh(*gskS.group_priv, gpkR);
  auto zz = zzER + zzSR;
  auto enc = group.serialize(*pkE);

  auto pkRm = group.serialize(gpkR);
  auto pkSm = group.serialize(*pkS);
  auto kem_context = enc + pkRm + pkSm;

  auto shared_secret = extract_and_expand(zz, kem_context);
  return std::make_pair(shared_secret, enc);
}

bytes
DHKEM::auth_decap(const bytes& enc,
                  const KEM::PublicKey& pkS,
                  const KEM::PrivateKey& skR) const
{
  const auto& gpkS = dynamic_cast<const Group::PublicKey&>(pkS);
  const auto& gskR = dynamic_cast<const PrivateKey&>(skR);

  auto pkE = group.deserialize(enc);
  auto pkR = gskR.group_priv->public_key();

  auto zzER = group.dh(*gskR.group_priv, *pkE);
  auto zzSR = group.dh(*gskR.group_priv, gpkS);
  auto zz = zzER + zzSR;

  auto pkRm = group.serialize(*pkR);
  auto pkSm = group.serialize(gpkS);
  auto kem_context = enc + pkRm + pkSm;

  return extract_and_expand(zz, kem_context);
}

bytes
DHKEM::extract_and_expand(const bytes& dh, const bytes& kem_context) const
{
  static const auto label_eae_prk = from_ascii("eae_prk");
  static const auto label_shared_secret = from_ascii("shared_secret");

  auto eae_prk = kdf.labeled_extract(suite_id, {}, label_eae_prk, dh);
  return kdf.labeled_expand(
    suite_id, eae_prk, label_shared_secret, kem_context, secret_size);
}

} // namespace mlspp::hpke
