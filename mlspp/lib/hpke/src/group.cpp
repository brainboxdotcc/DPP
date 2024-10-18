#include "group.h"

#include <hpke/random.h>

#include "common.h"
#include "openssl_common.h"

#include "openssl/bn.h"
#include "openssl/ec.h"
#include "openssl/evp.h"
#include "openssl/obj_mac.h"
#if defined(WITH_OPENSSL3)
#include "openssl/core_names.h"
#include "openssl/param_build.h"
#endif

namespace mlspp::hpke {

static inline size_t
group_dh_size(Group::ID group_id);

static inline const EVP_MD*
group_sig_digest(Group::ID group_id)
{
  switch (group_id) {
    case Group::ID::P256:
      return EVP_sha256();
    case Group::ID::P384:
      return EVP_sha384();
    case Group::ID::P521:
      return EVP_sha512();

    // EdDSA does its own hashing internally
    case Group::ID::Ed25519:
    case Group::ID::Ed448:
      return nullptr;

    // Groups not used for signature
    case Group::ID::X25519:
    case Group::ID::X448:
      throw std::runtime_error("Signature not supported for group");

    default:
      throw std::runtime_error("Unknown group");
  }
}

///
/// General implementation with OpenSSL EVP_PKEY
///

EVPGroup::EVPGroup(Group::ID group_id, const KDF& kdf)
  : Group(group_id, kdf)
{
}

EVPGroup::PublicKey::PublicKey(EVP_PKEY* pkey_in)
  : pkey(pkey_in, typed_delete<EVP_PKEY>)
{
}

EVPGroup::PrivateKey::PrivateKey(EVP_PKEY* pkey_in)
  : pkey(pkey_in, typed_delete<EVP_PKEY>)
{
}

std::unique_ptr<Group::PublicKey>
EVPGroup::PrivateKey::public_key() const
{
  if (1 != EVP_PKEY_up_ref(pkey.get())) {
    throw openssl_error();
  }
  return std::make_unique<PublicKey>(pkey.get());
}

std::unique_ptr<Group::PrivateKey>
EVPGroup::generate_key_pair() const
{
  return derive_key_pair({}, random_bytes(sk_size));
}

bytes
EVPGroup::dh(const Group::PrivateKey& sk, const Group::PublicKey& pk) const
{
  const auto& rsk = dynamic_cast<const PrivateKey&>(sk);
  const auto& rpk = dynamic_cast<const PublicKey&>(pk);

  // This and the next line are acceptable because the OpenSSL
  // functions fail to mark the required EVP_PKEYs as const, even
  // though they are not modified.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  auto* priv_pkey = const_cast<EVP_PKEY*>(rsk.pkey.get());
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  auto* pub_pkey = const_cast<EVP_PKEY*>(rpk.pkey.get());

  auto ctx = make_typed_unique(EVP_PKEY_CTX_new(priv_pkey, nullptr));
  if (ctx == nullptr) {
    throw openssl_error();
  }

  if (1 != EVP_PKEY_derive_init(ctx.get())) {
    throw openssl_error();
  }

  if (1 != EVP_PKEY_derive_set_peer(ctx.get(), pub_pkey)) {
    throw openssl_error();
  }

  size_t out_len = 0;
  if (1 != EVP_PKEY_derive(ctx.get(), nullptr, &out_len)) {
    throw openssl_error();
  }

  bytes out(out_len);
  uint8_t* ptr = out.data();
  if (1 != (EVP_PKEY_derive(ctx.get(), ptr, &out_len))) {
    throw openssl_error();
  }

  return out;
}

bytes
EVPGroup::sign(const bytes& data, const Group::PrivateKey& sk) const
{
  const auto& rsk = dynamic_cast<const PrivateKey&>(sk);

  auto ctx = make_typed_unique(EVP_MD_CTX_create());
  if (ctx == nullptr) {
    throw openssl_error();
  }

  const auto* digest = group_sig_digest(id);
  if (1 !=
      EVP_DigestSignInit(ctx.get(), nullptr, digest, nullptr, rsk.pkey.get())) {
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
EVPGroup::verify(const bytes& data,
                 const bytes& sig,
                 const Group::PublicKey& pk) const
{
  const auto& rpk = dynamic_cast<const PublicKey&>(pk);

  auto ctx = make_typed_unique(EVP_MD_CTX_create());
  if (ctx == nullptr) {
    throw openssl_error();
  }

  const auto* digest = group_sig_digest(id);
  if (1 != EVP_DigestVerifyInit(
             ctx.get(), nullptr, digest, nullptr, rpk.pkey.get())) {
    throw openssl_error();
  }

  auto rv = EVP_DigestVerify(
    ctx.get(), sig.data(), sig.size(), data.data(), data.size());

  return rv == 1;
}

///
/// DH over "normal" curves
///

struct ECKeyGroup : public EVPGroup
{
  ECKeyGroup(Group::ID group_id, const KDF& kdf)
    : EVPGroup(group_id, kdf)
    , curve_nid(group_to_nid(group_id))
  {
  }

#if defined(WITH_OPENSSL3)
  typed_unique_ptr<EVP_PKEY> keypair_evp_key(
    const typed_unique_ptr<BIGNUM>& priv) const
  {
    const auto* name = OBJ_nid2sn(curve_nid);
    if (name == nullptr) {
      throw std::runtime_error("Unsupported algorithm");
    }

    auto group = make_typed_unique(
      EC_GROUP_new_by_curve_name_ex(nullptr, nullptr, curve_nid));
    if (group == nullptr) {
      throw openssl_error();
    }

    auto pt = make_typed_unique(EC_POINT_new(group.get()));
    if (pt == nullptr) {
      throw openssl_error();
    }

    if (1 != EC_POINT_mul(
               group.get(), pt.get(), priv.get(), nullptr, nullptr, nullptr)) {
      throw openssl_error();
    }

    const auto pt_size = EC_POINT_point2oct(group.get(),
                                            pt.get(),
                                            POINT_CONVERSION_UNCOMPRESSED,
                                            nullptr,
                                            0,
                                            nullptr);
    if (0 == pt_size) {
      throw openssl_error();
    }

    bytes pub(pt_size);
    if (EC_POINT_point2oct(group.get(),
                           pt.get(),
                           POINT_CONVERSION_UNCOMPRESSED,
                           pub.data(),
                           pt_size,
                           nullptr) != pt_size) {
      throw openssl_error();
    }

    auto builder = make_typed_unique(OSSL_PARAM_BLD_new());
    if (builder == nullptr ||
        1 != OSSL_PARAM_BLD_push_utf8_string(
               builder.get(), OSSL_PKEY_PARAM_GROUP_NAME, name, 0) ||
        1 != OSSL_PARAM_BLD_push_BN(
               builder.get(), OSSL_PKEY_PARAM_PRIV_KEY, priv.get()) ||
        1 !=
          OSSL_PARAM_BLD_push_octet_string(
            builder.get(), OSSL_PKEY_PARAM_PUB_KEY, pub.data(), pub.size())) {
      throw openssl_error();
    }

    auto params = make_typed_unique(OSSL_PARAM_BLD_to_param(builder.get()));
    auto ctx =
      make_typed_unique(EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr));
    auto key = make_typed_unique(EVP_PKEY_new());
    auto* key_ptr = key.get();
    if (params == nullptr || ctx == nullptr || key == nullptr ||
        EVP_PKEY_fromdata_init(ctx.get()) <= 0 ||
        EVP_PKEY_fromdata(
          ctx.get(), &key_ptr, EVP_PKEY_KEYPAIR, params.get()) <= 0) {
      throw openssl_error();
    }
    ctx.reset();

    ctx = make_typed_unique(
      EVP_PKEY_CTX_new_from_pkey(nullptr, key.get(), nullptr));
    if (EVP_PKEY_check(ctx.get()) <= 0) {
      throw openssl_error();
    }

    return key;
  }

  typed_unique_ptr<EVP_PKEY> public_evp_key(const bytes& pub) const
  {
    const auto* name = OBJ_nid2sn(curve_nid);
    if (name == nullptr) {
      throw std::runtime_error("Unsupported algorithm");
    }

    auto group = make_typed_unique(
      EC_GROUP_new_by_curve_name_ex(nullptr, nullptr, curve_nid));
    if (group == nullptr) {
      throw openssl_error();
    }

    auto builder = make_typed_unique(OSSL_PARAM_BLD_new());
    if (builder == nullptr ||
        1 != OSSL_PARAM_BLD_push_utf8_string(
               builder.get(), OSSL_PKEY_PARAM_GROUP_NAME, name, 0) ||
        1 !=
          OSSL_PARAM_BLD_push_octet_string(
            builder.get(), OSSL_PKEY_PARAM_PUB_KEY, pub.data(), pub.size())) {
      throw openssl_error();
    }

    auto params = make_typed_unique(OSSL_PARAM_BLD_to_param(builder.get()));
    auto ctx =
      make_typed_unique(EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr));
    auto key = make_typed_unique(EVP_PKEY_new());
    auto* key_ptr = key.get();
    if (params == nullptr || ctx == nullptr || key == nullptr ||
        EVP_PKEY_fromdata_init(ctx.get()) <= 0 ||
        EVP_PKEY_fromdata(
          ctx.get(), &key_ptr, EVP_PKEY_KEYPAIR, params.get()) <= 0) {
      throw openssl_error();
    }
    ctx.reset();

    ctx = make_typed_unique(
      EVP_PKEY_CTX_new_from_pkey(nullptr, key.get(), nullptr));
    if (EVP_PKEY_public_check(ctx.get()) <= 0) {
      throw openssl_error();
    }

    return key;
  }
#endif

  std::unique_ptr<Group::PrivateKey> derive_key_pair(
    const bytes& suite_id,
    const bytes& ikm) const override
  {
    static const int retry_limit = 255;
    static const auto label_dkp_prk = from_ascii("dkp_prk");
    static const auto label_candidate = from_ascii("candidate");

    auto dkp_prk = kdf.labeled_extract(suite_id, {}, label_dkp_prk, ikm);

#if defined(WITH_OPENSSL3)
    auto* group = EC_GROUP_new_by_curve_name_ex(nullptr, nullptr, curve_nid);
    auto group_ptr = make_typed_unique(group);
#else
    auto eckey = new_ec_key();
    const auto* group = EC_KEY_get0_group(eckey.get());
#endif

    auto order = make_typed_unique(BN_new());
    if (1 != EC_GROUP_get_order(group, order.get(), nullptr)) {
      throw openssl_error();
    }

    auto sk = make_typed_unique(BN_new());
    BN_zero(sk.get());

    auto counter = int(0);
    while (BN_is_zero(sk.get()) != 0 || BN_cmp(sk.get(), order.get()) != -1) {
      auto ctr = i2osp(counter, 1);
      auto candidate =
        kdf.labeled_expand(suite_id, dkp_prk, label_candidate, ctr, sk_size);
      candidate.at(0) &= bitmask();
      sk.reset(BN_bin2bn(
        candidate.data(), static_cast<int>(candidate.size()), nullptr));

      counter += 1;
      if (counter > retry_limit) {
        throw std::runtime_error("DeriveKeyPair iteration limit exceeded");
      }
    }

#if defined(WITH_OPENSSL3)
    auto key = keypair_evp_key(sk);
    return std::make_unique<EVPGroup::PrivateKey>(key.release());
#else
    auto pt = make_typed_unique(EC_POINT_new(group));
    EC_POINT_mul(group, pt.get(), sk.get(), nullptr, nullptr, nullptr);

    EC_KEY_set_private_key(eckey.get(), sk.get());
    EC_KEY_set_public_key(eckey.get(), pt.get());

    auto pkey = to_pkey(eckey.release());
    return std::make_unique<PrivateKey>(pkey.release());
#endif
  }

  bytes serialize(const Group::PublicKey& pk) const override
  {
    const auto& rpk = dynamic_cast<const PublicKey&>(pk);
#if defined(WITH_OPENSSL3)
    OSSL_PARAM* param = nullptr;
    if (1 != EVP_PKEY_todata(rpk.pkey.get(), EVP_PKEY_PUBLIC_KEY, &param)) {
      throw openssl_error();
    }
    auto param_ptr = make_typed_unique(param);

    const OSSL_PARAM* pk_param =
      OSSL_PARAM_locate_const(param_ptr.get(), OSSL_PKEY_PARAM_PUB_KEY);
    if (pk_param == nullptr) {
      return bytes({}, 0);
    }

    size_t len = 0;
    if (1 != OSSL_PARAM_get_octet_string(pk_param, nullptr, 0, &len)) {
      return bytes({}, 0);
    }

    bytes buf(len);
    void* data_ptr = buf.data();
    if (1 != OSSL_PARAM_get_octet_string(pk_param, &data_ptr, len, nullptr)) {
      return bytes({}, 0);
    }

    // Prior to OpenSSL 3.0.8, we will always get compressed point from
    // OSSL_PKEY_PARAM_PUB_KEY, so we will have to do the following conversion.
    // From OpenSSL 3.0.8, we can obtain the uncompressed point value by setting
    // OSSL_PKEY_PARAM_EC_POINT_CONVERSION_FORMAT appropriately.
    auto group = make_typed_unique(
      EC_GROUP_new_by_curve_name_ex(nullptr, nullptr, curve_nid));
    if (group == nullptr) {
      return bytes({}, 0);
    }
    auto point = make_typed_unique(EC_POINT_new(group.get()));
    const auto* oct_ptr = static_cast<const unsigned char*>(data_ptr);
    if (1 !=
        EC_POINT_oct2point(group.get(), point.get(), oct_ptr, len, nullptr)) {
      return bytes({}, 0);
    }
    len = EC_POINT_point2oct(group.get(),
                             point.get(),
                             POINT_CONVERSION_UNCOMPRESSED,
                             nullptr,
                             0,
                             nullptr);
    if (0 == len) {
      return bytes({}, 0);
    }
    bytes out(len);
    auto* data = out.data();
    if (EC_POINT_point2oct(group.get(),
                           point.get(),
                           POINT_CONVERSION_UNCOMPRESSED,
                           data,
                           len,
                           nullptr) != len) {
      return bytes({}, 0);
    }
#else
    auto* pub = EVP_PKEY_get0_EC_KEY(rpk.pkey.get());

    auto len = i2o_ECPublicKey(pub, nullptr);
    if (len != static_cast<int>(pk_size)) {
      throw openssl_error();
    }

    bytes out(len);
    auto* data = out.data();
    if (i2o_ECPublicKey(pub, &data) == 0) {
      throw openssl_error();
    }
#endif
    return out;
  }

  std::unique_ptr<Group::PublicKey> deserialize(const bytes& enc) const override
  {
#if defined(WITH_OPENSSL3)
    auto key = public_evp_key(enc);
    if (key == nullptr) {
      throw std::runtime_error("Unable to deserialize the public key");
    }
    return std::make_unique<EVPGroup::PublicKey>(key.release());
#else
    auto eckey = new_ec_key();
    auto* eckey_ptr = eckey.get();
    const auto* data_ptr = enc.data();
    if (nullptr ==
        o2i_ECPublicKey(&eckey_ptr,
                        &data_ptr,
                        static_cast<long>( // NOLINT(google-runtime-int)
                          enc.size()))) {
      throw openssl_error();
    }

    auto pkey = to_pkey(eckey.release());
    return std::make_unique<EVPGroup::PublicKey>(pkey.release());
#endif
  }

  bytes serialize_private(const Group::PrivateKey& sk) const override
  {
    const auto& rsk = dynamic_cast<const PrivateKey&>(sk);
#if defined(WITH_OPENSSL3)
    OSSL_PARAM* param = nullptr;
    if (1 != EVP_PKEY_todata(rsk.pkey.get(), EVP_PKEY_KEYPAIR, &param)) {
      throw openssl_error();
    }
    auto param_ptr = make_typed_unique(param);

    const OSSL_PARAM* sk_param =
      OSSL_PARAM_locate_const(param_ptr.get(), OSSL_PKEY_PARAM_PRIV_KEY);
    if (sk_param == nullptr) {
      return bytes({}, 0);
    }

    BIGNUM* d = nullptr;
    if (1 != OSSL_PARAM_get_BN(sk_param, &d)) {
      return bytes({}, 0);
    }
    auto d_ptr = make_typed_unique(d);
#else
    auto* eckey = EVP_PKEY_get0_EC_KEY(rsk.pkey.get());
    const auto* d = EC_KEY_get0_private_key(eckey);
#endif

    auto out = bytes(BN_num_bytes(d));
#if WITH_BORINGSSL
    // In BoringSSL, BN_bn2bin returns size_t
    const auto out_size = out.size();
#else
    // In OpenSSL, BN_bn2bin returns int
    const auto out_size = static_cast<int>(out.size());
#endif
    if (BN_bn2bin(d, out.data()) != out_size) {
      throw openssl_error();
    }

    const auto zeros_needed = group_dh_size(id) - out.size();
    auto leading_zeros = bytes(zeros_needed, 0);
    return leading_zeros + out;
  }

  std::unique_ptr<Group::PrivateKey> deserialize_private(
    const bytes& skm) const override
  {
#if defined(WITH_OPENSSL3)
    auto priv = make_typed_unique(
      BN_bin2bn(skm.data(), static_cast<int>(skm.size()), nullptr));
    if (priv == nullptr) {
      throw std::runtime_error("Unable to deserialize the private key");
    }
    auto key = keypair_evp_key(priv);
    return std::make_unique<EVPGroup::PrivateKey>(key.release());
#else
    auto eckey = new_ec_key();
    const auto* group = EC_KEY_get0_group(eckey.get());
    const auto d = make_typed_unique(
      BN_bin2bn(skm.data(), static_cast<int>(skm.size()), nullptr));
    auto pt = make_typed_unique(EC_POINT_new(group));

    EC_POINT_mul(group, pt.get(), d.get(), nullptr, nullptr, nullptr);
    EC_KEY_set_private_key(eckey.get(), d.get());
    EC_KEY_set_public_key(eckey.get(), pt.get());

    auto pkey = to_pkey(eckey.release());
    return std::make_unique<EVPGroup::PrivateKey>(pkey.release());
#endif
  }

  // EC Key
  std::tuple<bytes, bytes> coordinates(
    const Group::PublicKey& pk) const override
  {
    auto bn_x = make_typed_unique(BN_new());
    auto bn_y = make_typed_unique(BN_new());
    const auto& rpk = dynamic_cast<const PublicKey&>(pk);

#if defined(WITH_OPENSSL3)
    // Raw pointer OK here because it becomes managed as soon as possible
    OSSL_PARAM* param_ptr = nullptr;
    if (1 != EVP_PKEY_todata(rpk.pkey.get(), EVP_PKEY_PUBLIC_KEY, &param_ptr)) {
      throw openssl_error();
    }

    auto param = make_typed_unique(param_ptr);

    // Raw pointer OK here because it is non-owning
    const auto* pk_param =
      OSSL_PARAM_locate_const(param.get(), OSSL_PKEY_PARAM_PUB_KEY);
    if (pk_param == nullptr) {
      throw std::runtime_error("Failed to locate OSSL_PKEY_PARAM_PUB_KEY");
    }

    // Copy the octet string representation of the key into a buffer
    auto len = size_t(0);
    if (1 != OSSL_PARAM_get_octet_string(pk_param, nullptr, 0, &len)) {
      throw std::runtime_error("Failed to get OSSL_PKEY_PARAM_PUB_KEY len");
    }

    auto buf = bytes(len);
    auto* buf_ptr = buf.data();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* buf_ptr_void = reinterpret_cast<void*>(buf_ptr);
    if (1 !=
        OSSL_PARAM_get_octet_string(pk_param, &buf_ptr_void, len, nullptr)) {
      throw std::runtime_error("Failed to get OSSL_PKEY_PARAM_PUB_KEY data");
    }

    // Parse the octet string representation into an EC_POINT
    auto group = make_typed_unique(
      EC_GROUP_new_by_curve_name_ex(nullptr, nullptr, curve_nid));
    if (group == nullptr) {
      throw openssl_error();
    }

    auto point = make_typed_unique(EC_POINT_new(group.get()));
    if (point == nullptr) {
      throw openssl_error();
    }

    if (1 !=
        EC_POINT_oct2point(group.get(), point.get(), buf_ptr, len, nullptr)) {
      throw openssl_error();
    }

    // Retrieve the affine coordinates of the point
    if (1 != EC_POINT_get_affine_coordinates(
               group.get(), point.get(), bn_x.get(), bn_y.get(), nullptr)) {
      throw openssl_error();
    }
#else
    // Raw pointers are non-owning
    auto* pub = EVP_PKEY_get0_EC_KEY(rpk.pkey.get());
    const auto* point = EC_KEY_get0_public_key(pub);
    const auto* group = EC_KEY_get0_group(pub);

    if (1 != EC_POINT_get_affine_coordinates_GFp(
               group, point, bn_x.get(), bn_y.get(), nullptr)) {
      throw openssl_error();
    }
#endif
    const auto x_size = BN_num_bytes(bn_x.get());
    auto x = bytes(x_size);
    if (BN_bn2bin(bn_x.get(), x.data()) != x_size) {
      throw openssl_error();
    }

    const auto y_size = BN_num_bytes(bn_y.get());
    auto y = bytes(y_size);
    if (BN_bn2bin(bn_y.get(), y.data()) != y_size) {
      throw openssl_error();
    }

    const auto zeros_needed_x = dh_size - x.size();
    const auto zeros_needed_y = dh_size - y.size();
    auto leading_zeros_x = bytes(zeros_needed_x, 0);
    auto leading_zeros_y = bytes(zeros_needed_y, 0);

    return { leading_zeros_x + x, leading_zeros_y + y };
  }

  // EC Key
  std::unique_ptr<Group::PublicKey> public_key_from_coordinates(
    const bytes& x,
    const bytes& y) const override
  {
    auto bn_x = make_typed_unique(
      BN_bin2bn(x.data(), static_cast<int>(x.size()), nullptr));
    auto bn_y = make_typed_unique(
      BN_bin2bn(y.data(), static_cast<int>(y.size()), nullptr));

    if (bn_x == nullptr || bn_y == nullptr) {
      throw std::runtime_error("Failed to convert bn_x or bn_y");
    }

#if defined(WITH_OPENSSL3)
    const auto group = make_typed_unique(
      EC_GROUP_new_by_curve_name_ex(nullptr, nullptr, curve_nid));
    if (group == nullptr) {
      throw std::runtime_error("Failed to create EC_GROUP");
    }

    // Construct a point with the given coordinates
    auto point = make_typed_unique(EC_POINT_new(group.get()));
    if (group == nullptr) {
      throw std::runtime_error("Failed to create EC_POINT");
    }

    if (1 != EC_POINT_set_affine_coordinates(
               group.get(), point.get(), bn_x.get(), bn_y.get(), nullptr)) {
      throw openssl_error();
    }

    // Serialize the point
    const auto point_size = EC_POINT_point2oct(group.get(),
                                               point.get(),
                                               POINT_CONVERSION_UNCOMPRESSED,
                                               nullptr,
                                               0,
                                               nullptr);
    if (0 == point_size) {
      throw openssl_error();
    }

    auto pub = bytes(point_size);
    if (EC_POINT_point2oct(group.get(),
                           point.get(),
                           POINT_CONVERSION_UNCOMPRESSED,
                           pub.data(),
                           point_size,
                           nullptr) != point_size) {
      throw openssl_error();
    }

    // Initialize a public key from the serialized point
    auto key = public_evp_key(pub);
    return std::make_unique<EVPGroup::PublicKey>(key.release());
#else
    auto eckey = new_ec_key();
    if (eckey == nullptr) {
      throw std::runtime_error("Failed to create EC_KEY");
    }

    // Group pointer is non-owning
    const auto* group = EC_KEY_get0_group(eckey.get());
    auto point = make_typed_unique(EC_POINT_new(group));

    if (1 != EC_POINT_set_affine_coordinates_GFp(
               group, point.get(), bn_x.get(), bn_y.get(), nullptr)) {
      throw openssl_error();
    }

    if (1 != EC_KEY_set_public_key(eckey.get(), point.get())) {
      throw openssl_error();
    }

    auto pkey = to_pkey(eckey.release());
    return std::make_unique<EVPGroup::PublicKey>(pkey.release());
#endif
  }

private:
  int curve_nid;

#if !defined(WITH_OPENSSL3)
  typed_unique_ptr<EC_KEY> new_ec_key() const
  {
    return make_typed_unique(EC_KEY_new_by_curve_name(curve_nid));
  }

  static typed_unique_ptr<EVP_PKEY> to_pkey(EC_KEY* eckey)
  {
    auto* pkey = EVP_PKEY_new();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    EVP_PKEY_assign_EC_KEY(pkey, eckey);
    return make_typed_unique(pkey);
  }
#endif

  static inline int group_to_nid(Group::ID group_id)
  {
    switch (group_id) {
      case Group::ID::P256:
        return NID_X9_62_prime256v1;
      case Group::ID::P384:
        return NID_secp384r1;
      case Group::ID::P521:
        return NID_secp521r1;
      default:
        throw std::runtime_error("Unsupported algorithm");
    }
  }

  uint8_t bitmask() const
  {
    switch (id) {
      case Group::ID::P256:
      case Group::ID::P384:
        return 0xff;

      case Group::ID::P521:
        return 0x01;

      default:
        throw std::runtime_error("Unsupported algorithm");
    }
  }
};

///
/// DH over "raw" curves
///

struct RawKeyGroup : public EVPGroup
{
  RawKeyGroup(Group::ID group_id, const KDF& kdf)
    : EVPGroup(group_id, kdf)
    , evp_type(group_to_evp(group_id))
  {
  }

  template<Group::ID id>
  static const RawKeyGroup instance;

  std::unique_ptr<Group::PrivateKey> derive_key_pair(
    const bytes& suite_id,
    const bytes& ikm) const override
  {
    static const auto label_dkp_prk = from_ascii("dkp_prk");
    static const auto label_sk = from_ascii("sk");

    auto dkp_prk = kdf.labeled_extract(suite_id, {}, label_dkp_prk, ikm);
    auto skm = kdf.labeled_expand(suite_id, dkp_prk, label_sk, {}, sk_size);
    return deserialize_private(skm);
  }

  bytes serialize(const Group::PublicKey& pk) const override
  {
    const auto& rpk = dynamic_cast<const PublicKey&>(pk);
    auto raw = bytes(pk_size);
    auto* data_ptr = raw.data();
    auto data_len = raw.size();
    if (1 != EVP_PKEY_get_raw_public_key(rpk.pkey.get(), data_ptr, &data_len)) {
      throw openssl_error();
    }

    return raw;
  }

  std::unique_ptr<Group::PublicKey> deserialize(const bytes& enc) const override
  {
    auto* pkey =
      EVP_PKEY_new_raw_public_key(evp_type, nullptr, enc.data(), enc.size());
    if (pkey == nullptr) {
      throw openssl_error();
    }

    return std::make_unique<EVPGroup::PublicKey>(pkey);
  }

  bytes serialize_private(const Group::PrivateKey& sk) const override
  {
    const auto& rsk = dynamic_cast<const PrivateKey&>(sk);
    auto raw = bytes(sk_size);
    auto* data_ptr = raw.data();
    auto data_len = raw.size();
    if (1 !=
        EVP_PKEY_get_raw_private_key(rsk.pkey.get(), data_ptr, &data_len)) {
      throw openssl_error();
    }

    return raw;
  }

  std::unique_ptr<Group::PrivateKey> deserialize_private(
    const bytes& skm) const override
  {
    auto* pkey =
      EVP_PKEY_new_raw_private_key(evp_type, nullptr, skm.data(), skm.size());
    if (pkey == nullptr) {
      throw openssl_error();
    }

    return std::make_unique<EVPGroup::PrivateKey>(pkey);
  }

  // Raw Key
  std::tuple<bytes, bytes> coordinates(
    const Group::PublicKey& pk) const override
  {
    const auto& rpk = dynamic_cast<const PublicKey&>(pk);
    auto raw = bytes(pk_size);
    auto* data_ptr = raw.data();
    auto data_len = raw.size();

    if (1 != EVP_PKEY_get_raw_public_key(rpk.pkey.get(), data_ptr, &data_len)) {
      throw openssl_error();
    }

    return { raw, {} };
  }

  // Raw Key
  std::unique_ptr<Group::PublicKey> public_key_from_coordinates(
    const bytes& x,
    const bytes& /* y */) const override
  {
    return deserialize(x);
  }

private:
  const int evp_type;

  static inline int group_to_evp(Group::ID group_id)
  {
    switch (group_id) {
      case Group::ID::X25519:
        return EVP_PKEY_X25519;
      case Group::ID::X448:
        return EVP_PKEY_X448;
      case Group::ID::Ed25519:
        return EVP_PKEY_ED25519;
      case Group::ID::Ed448:
        return EVP_PKEY_ED448;
      default:
        throw std::runtime_error("Unsupported algorithm");
    }
  }
};

///
/// General DH group
///

template<>
const Group&
Group::get<Group::ID::P256>()
{
  static const ECKeyGroup instance(Group::ID::P256,
                                   KDF::get<KDF::ID::HKDF_SHA256>());

  return instance;
}

template<>
const Group&
Group::get<Group::ID::P384>()
{
  static const ECKeyGroup instance(Group::ID::P384,
                                   KDF::get<KDF::ID::HKDF_SHA384>());

  return instance;
}

template<>
const Group&
Group::get<Group::ID::P521>()
{
  static const ECKeyGroup instance(Group::ID::P521,
                                   KDF::get<KDF::ID::HKDF_SHA512>());

  return instance;
}

template<>
const Group&
Group::get<Group::ID::X25519>()
{
  static const RawKeyGroup instance(Group::ID::X25519,
                                    KDF::get<KDF::ID::HKDF_SHA256>());
  return instance;
}

template<>
const Group&
Group::get<Group::ID::Ed25519>()
{
  static const RawKeyGroup instance(Group::ID::Ed25519,
                                    KDF::get<KDF::ID::HKDF_SHA256>());
  return instance;
}

// BoringSSL doesn't support X448 / Ed448
#if !defined(WITH_BORINGSSL)
template<>
const Group&
Group::get<Group::ID::X448>()
{
  static const RawKeyGroup instance(Group::ID::X448,
                                    KDF::get<KDF::ID::HKDF_SHA512>());
  return instance;
}
#endif

template<>
const Group&
Group::get<Group::ID::Ed448>()
{
  static const RawKeyGroup instance(Group::ID::Ed448,
                                    KDF::get<KDF::ID::HKDF_SHA512>());
  return instance;
}

static inline size_t
group_dh_size(Group::ID group_id)
{
  switch (group_id) {
    case Group::ID::P256:
      return 32;
    case Group::ID::P384:
      return 48;
    case Group::ID::P521:
      return 66;
    case Group::ID::X25519:
      return 32;
    case Group::ID::X448:
      return 56;

    // Non-DH groups
    case Group::ID::Ed25519:
    case Group::ID::Ed448:
      return 0;

    default:
      throw std::runtime_error("Unknown group");
  }
}

static inline size_t
group_pk_size(Group::ID group_id)
{
  switch (group_id) {
    case Group::ID::P256:
      return 65;
    case Group::ID::P384:
      return 97;
    case Group::ID::P521:
      return 133;
    case Group::ID::X25519:
    case Group::ID::Ed25519:
      return 32;
    case Group::ID::X448:
      return 56;
    case Group::ID::Ed448:
      return 57;

    default:
      throw std::runtime_error("Unknown group");
  }
}

static inline size_t
group_sk_size(Group::ID group_id)
{
  switch (group_id) {
    case Group::ID::P256:
      return 32;
    case Group::ID::P384:
      return 48;
    case Group::ID::P521:
      return 66;
    case Group::ID::X25519:
    case Group::ID::Ed25519:
      return 32;
    case Group::ID::X448:
      return 56;
    case Group::ID::Ed448:
      return 57;

    default:
      throw std::runtime_error("Unknown group");
  }
}

static inline std::string
group_jwk_curve_name(Group::ID group_id)
{
  switch (group_id) {
    case Group::ID::P256:
      return "P-256";
    case Group::ID::P384:
      return "P-384";
    case Group::ID::P521:
      return "P-521";
    case Group::ID::Ed25519:
      return "Ed25519";
    case Group::ID::Ed448:
      return "Ed448";
    case Group::ID::X25519:
      return "X25519";
    case Group::ID::X448:
      return "X448";
    default:
      throw std::runtime_error("Unknown group");
  }
}

static inline std::string
group_jwk_key_type(Group::ID group_id)
{
  switch (group_id) {
    case Group::ID::P256:
    case Group::ID::P384:
    case Group::ID::P521:
      return "EC";
    case Group::ID::Ed25519:
    case Group::ID::Ed448:
    case Group::ID::X25519:
    case Group::ID::X448:
      return "OKP";
    default:
      throw std::runtime_error("Unknown group");
  }
}

Group::Group(ID group_id_in, const KDF& kdf_in)
  : id(group_id_in)
  , dh_size(group_dh_size(group_id_in))
  , pk_size(group_pk_size(group_id_in))
  , sk_size(group_sk_size(group_id_in))
  , jwk_key_type(group_jwk_key_type(group_id_in))
  , jwk_curve_name(group_jwk_curve_name(group_id_in))
  , kdf(kdf_in)
{
}

} // namespace mlspp::hpke
