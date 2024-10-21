#include <mls/core_types.h>
#include <mls/crypto.h>
#include <mls/messages.h>

#include <string>

using mlspp::hpke::AEAD;      // NOLINT(misc-unused-using-decls)
using mlspp::hpke::Digest;    // NOLINT(misc-unused-using-decls)
using mlspp::hpke::HPKE;      // NOLINT(misc-unused-using-decls)
using mlspp::hpke::KDF;       // NOLINT(misc-unused-using-decls)
using mlspp::hpke::KEM;       // NOLINT(misc-unused-using-decls)
using mlspp::hpke::Signature; // NOLINT(misc-unused-using-decls)

namespace mlspp {

SignatureScheme
tls_signature_scheme(Signature::ID id)
{
  switch (id) {
    case Signature::ID::P256_SHA256:
      return SignatureScheme::ecdsa_secp256r1_sha256;
    case Signature::ID::P384_SHA384:
      return SignatureScheme::ecdsa_secp384r1_sha384;
    case Signature::ID::P521_SHA512:
      return SignatureScheme::ecdsa_secp521r1_sha512;
    case Signature::ID::Ed25519:
      return SignatureScheme::ed25519;
#if !defined(WITH_BORINGSSL)
    case Signature::ID::Ed448:
      return SignatureScheme::ed448;
#endif
    case Signature::ID::RSA_SHA256:
      return SignatureScheme::rsa_pkcs1_sha256;
    default:
      throw InvalidParameterError("Unsupported algorithm");
  }
}

///
/// CipherSuites and details
///

CipherSuite::CipherSuite()
  : id(ID::unknown)
{
}

CipherSuite::CipherSuite(ID id_in)
  : id(id_in)
{
}

SignatureScheme
CipherSuite::signature_scheme() const
{
  switch (id) {
    case ID::X25519_AES128GCM_SHA256_Ed25519:
    case ID::X25519_CHACHA20POLY1305_SHA256_Ed25519:
      return SignatureScheme::ed25519;
    case ID::P256_AES128GCM_SHA256_P256:
      return SignatureScheme::ecdsa_secp256r1_sha256;
    case ID::X448_AES256GCM_SHA512_Ed448:
    case ID::X448_CHACHA20POLY1305_SHA512_Ed448:
      return SignatureScheme::ed448;
    case ID::P521_AES256GCM_SHA512_P521:
      return SignatureScheme::ecdsa_secp521r1_sha512;
    case ID::P384_AES256GCM_SHA384_P384:
      return SignatureScheme::ecdsa_secp384r1_sha384;
    default:
      throw InvalidParameterError("Unsupported algorithm");
  }
}

const CipherSuite::Ciphers&
CipherSuite::get() const
{
  static const auto ciphers_X25519_AES128GCM_SHA256_Ed25519 =
    CipherSuite::Ciphers{
      HPKE(KEM::ID::DHKEM_X25519_SHA256,
           KDF::ID::HKDF_SHA256,
           AEAD::ID::AES_128_GCM),
      Digest::get<Digest::ID::SHA256>(),
      Signature::get<Signature::ID::Ed25519>(),
    };

  static const auto ciphers_P256_AES128GCM_SHA256_P256 = CipherSuite::Ciphers{
    HPKE(
      KEM::ID::DHKEM_P256_SHA256, KDF::ID::HKDF_SHA256, AEAD::ID::AES_128_GCM),
    Digest::get<Digest::ID::SHA256>(),
    Signature::get<Signature::ID::P256_SHA256>(),
  };

  static const auto ciphers_X25519_CHACHA20POLY1305_SHA256_Ed25519 =
    CipherSuite::Ciphers{
      HPKE(KEM::ID::DHKEM_X25519_SHA256,
           KDF::ID::HKDF_SHA256,
           AEAD::ID::CHACHA20_POLY1305),
      Digest::get<Digest::ID::SHA256>(),
      Signature::get<Signature::ID::Ed25519>(),
    };

  static const auto ciphers_P521_AES256GCM_SHA512_P521 = CipherSuite::Ciphers{
    HPKE(
      KEM::ID::DHKEM_P521_SHA512, KDF::ID::HKDF_SHA512, AEAD::ID::AES_256_GCM),
    Digest::get<Digest::ID::SHA512>(),
    Signature::get<Signature::ID::P521_SHA512>(),
  };

  static const auto ciphers_P384_AES256GCM_SHA384_P384 = CipherSuite::Ciphers{
    HPKE(
      KEM::ID::DHKEM_P384_SHA384, KDF::ID::HKDF_SHA384, AEAD::ID::AES_256_GCM),
    Digest::get<Digest::ID::SHA384>(),
    Signature::get<Signature::ID::P384_SHA384>(),
  };

#if !defined(WITH_BORINGSSL)
  static const auto ciphers_X448_AES256GCM_SHA512_Ed448 = CipherSuite::Ciphers{
    HPKE(
      KEM::ID::DHKEM_X448_SHA512, KDF::ID::HKDF_SHA512, AEAD::ID::AES_256_GCM),
    Digest::get<Digest::ID::SHA512>(),
    Signature::get<Signature::ID::Ed448>(),
  };

  static const auto ciphers_X448_CHACHA20POLY1305_SHA512_Ed448 =
    CipherSuite::Ciphers{
      HPKE(KEM::ID::DHKEM_X448_SHA512,
           KDF::ID::HKDF_SHA512,
           AEAD::ID::CHACHA20_POLY1305),
      Digest::get<Digest::ID::SHA512>(),
      Signature::get<Signature::ID::Ed448>(),
    };
#endif

  switch (id) {
    case ID::unknown:
      throw InvalidParameterError("Uninitialized ciphersuite");

    case ID::X25519_AES128GCM_SHA256_Ed25519:
      return ciphers_X25519_AES128GCM_SHA256_Ed25519;

    case ID::P256_AES128GCM_SHA256_P256:
      return ciphers_P256_AES128GCM_SHA256_P256;

    case ID::X25519_CHACHA20POLY1305_SHA256_Ed25519:
      return ciphers_X25519_CHACHA20POLY1305_SHA256_Ed25519;

    case ID::P521_AES256GCM_SHA512_P521:
      return ciphers_P521_AES256GCM_SHA512_P521;

    case ID::P384_AES256GCM_SHA384_P384:
      return ciphers_P384_AES256GCM_SHA384_P384;

#if !defined(WITH_BORINGSSL)
    case ID::X448_AES256GCM_SHA512_Ed448:
      return ciphers_X448_AES256GCM_SHA512_Ed448;

    case ID::X448_CHACHA20POLY1305_SHA512_Ed448:
      return ciphers_X448_CHACHA20POLY1305_SHA512_Ed448;
#endif

    default:
      throw InvalidParameterError("Unsupported ciphersuite");
  }
}

struct HKDFLabel
{
  uint16_t length;
  bytes label;
  bytes context;

  TLS_SERIALIZABLE(length, label, context)
};

bytes
CipherSuite::expand_with_label(const bytes& secret,
                               const std::string& label,
                               const bytes& context,
                               size_t length) const
{
  auto mls_label = from_ascii(std::string("MLS 1.0 ") + label);
  auto length16 = static_cast<uint16_t>(length);
  auto label_bytes = tls::marshal(HKDFLabel{ length16, mls_label, context });
  return get().hpke.kdf.expand(secret, label_bytes, length);
}

bytes
CipherSuite::derive_secret(const bytes& secret, const std::string& label) const
{
  return expand_with_label(secret, label, {}, secret_size());
}

bytes
CipherSuite::derive_tree_secret(const bytes& secret,
                                const std::string& label,
                                uint32_t generation,
                                size_t length) const
{
  return expand_with_label(secret, label, tls::marshal(generation), length);
}

#if WITH_BORINGSSL
const std::array<CipherSuite::ID, 5> all_supported_suites = {
  CipherSuite::ID::X25519_AES128GCM_SHA256_Ed25519,
  CipherSuite::ID::P256_AES128GCM_SHA256_P256,
  CipherSuite::ID::X25519_CHACHA20POLY1305_SHA256_Ed25519,
  CipherSuite::ID::P521_AES256GCM_SHA512_P521,
  CipherSuite::ID::P384_AES256GCM_SHA384_P384,
};
#else
const std::array<CipherSuite::ID, 7> all_supported_suites = {
  CipherSuite::ID::X25519_AES128GCM_SHA256_Ed25519,
  CipherSuite::ID::P256_AES128GCM_SHA256_P256,
  CipherSuite::ID::X25519_CHACHA20POLY1305_SHA256_Ed25519,
  CipherSuite::ID::P521_AES256GCM_SHA512_P521,
  CipherSuite::ID::P384_AES256GCM_SHA384_P384,
  CipherSuite::ID::X448_CHACHA20POLY1305_SHA512_Ed448,
  CipherSuite::ID::X448_AES256GCM_SHA512_Ed448,
};
#endif

// MakeKeyPackageRef(value) = KDF.expand(
//   KDF.extract("", value), "MLS 1.0 KeyPackage Reference", 16)
template<>
const bytes&
CipherSuite::reference_label<KeyPackage>()
{
  static const auto label = from_ascii("MLS 1.0 KeyPackage Reference");
  return label;
}

// MakeProposalRef(value) = KDF.expand(
//   KDF.extract("", value), "MLS 1.0 Proposal Reference", 16)
//
// Even though the label says "Proposal", we actually hash the entire enclosing
// AuthenticatedContent object.
template<>
const bytes&
CipherSuite::reference_label<AuthenticatedContent>()
{
  static const auto label = from_ascii("MLS 1.0 Proposal Reference");
  return label;
}

///
/// HPKEPublicKey and HPKEPrivateKey
///

// This function produces a non-literal type, so it can't be constexpr.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MLS_1_0_PLUS(label) from_ascii("MLS 1.0 " label)

static bytes
mls_1_0_plus(const std::string& label)
{
  auto plus = "MLS 1.0 "s + label;
  return from_ascii(plus);
}

namespace encrypt_label {
const std::string update_path_node = "UpdatePathNode";
const std::string welcome = "Welcome";
} // namespace encrypt_label

struct EncryptContext
{
  const bytes& label;
  const bytes& content;
  TLS_SERIALIZABLE(label, content)
};

HPKECiphertext
HPKEPublicKey::encrypt(CipherSuite suite,
                       const std::string& label,
                       const bytes& context,
                       const bytes& pt) const
{
  auto label_plus = mls_1_0_plus(label);
  auto encrypt_context = tls::marshal(EncryptContext{ label_plus, context });
  auto pkR = suite.hpke().kem.deserialize(data);
  auto [enc, ctx] = suite.hpke().setup_base_s(*pkR, encrypt_context);
  auto ct = ctx.seal({}, pt);
  return HPKECiphertext{ enc, ct };
}

std::tuple<bytes, bytes>
HPKEPublicKey::do_export(CipherSuite suite,
                         const bytes& info,
                         const std::string& label,
                         size_t size) const
{
  auto label_data = from_ascii(label);
  auto pkR = suite.hpke().kem.deserialize(data);
  auto [enc, ctx] = suite.hpke().setup_base_s(*pkR, info);
  auto exported = ctx.do_export(label_data, size);
  return std::make_tuple(enc, exported);
}

HPKEPrivateKey
HPKEPrivateKey::generate(CipherSuite suite)
{
  auto priv = suite.hpke().kem.generate_key_pair();
  auto priv_data = suite.hpke().kem.serialize_private(*priv);
  auto pub = priv->public_key();
  auto pub_data = suite.hpke().kem.serialize(*pub);
  return { priv_data, pub_data };
}

HPKEPrivateKey
HPKEPrivateKey::parse(CipherSuite suite, const bytes& data)
{
  auto priv = suite.hpke().kem.deserialize_private(data);
  auto pub = priv->public_key();
  auto pub_data = suite.hpke().kem.serialize(*pub);
  return { data, pub_data };
}

HPKEPrivateKey
HPKEPrivateKey::derive(CipherSuite suite, const bytes& secret)
{
  auto priv = suite.hpke().kem.derive_key_pair(secret);
  auto priv_data = suite.hpke().kem.serialize_private(*priv);
  auto pub = priv->public_key();
  auto pub_data = suite.hpke().kem.serialize(*pub);
  return { priv_data, pub_data };
}

bytes
HPKEPrivateKey::decrypt(CipherSuite suite,
                        const std::string& label,
                        const bytes& context,
                        const HPKECiphertext& ct) const
{
  auto label_plus = mls_1_0_plus(label);
  auto encrypt_context = tls::marshal(EncryptContext{ label_plus, context });
  auto skR = suite.hpke().kem.deserialize_private(data);
  auto ctx = suite.hpke().setup_base_r(ct.kem_output, *skR, encrypt_context);
  auto pt = ctx.open({}, ct.ciphertext);
  if (!pt) {
    throw InvalidParameterError("HPKE decryption failure");
  }

  return opt::get(pt);
}

bytes
HPKEPrivateKey::do_export(CipherSuite suite,
                          const bytes& info,
                          const bytes& kem_output,
                          const std::string& label,
                          size_t size) const
{
  auto label_data = from_ascii(label);
  auto skR = suite.hpke().kem.deserialize_private(data);
  auto ctx = suite.hpke().setup_base_r(kem_output, *skR, info);
  return ctx.do_export(label_data, size);
}

HPKEPrivateKey::HPKEPrivateKey(bytes priv_data, bytes pub_data)
  : data(std::move(priv_data))
  , public_key{ std::move(pub_data) }
{
}

void
HPKEPrivateKey::set_public_key(CipherSuite suite)
{
  const auto priv = suite.hpke().kem.deserialize_private(data);
  auto pub = priv->public_key();
  public_key.data = suite.hpke().kem.serialize(*pub);
}

///
/// SignaturePublicKey and SignaturePrivateKey
///
namespace sign_label {
const std::string mls_content = "FramedContentTBS";
const std::string leaf_node = "LeafNodeTBS";
const std::string key_package = "KeyPackageTBS";
const std::string group_info = "GroupInfoTBS";
const std::string multi_credential = "MultiCredential";
} // namespace sign_label

struct SignContent
{
  const bytes& label;
  const bytes& content;
  TLS_SERIALIZABLE(label, content)
};

bool
SignaturePublicKey::verify(const CipherSuite& suite,
                           const std::string& label,
                           const bytes& message,
                           const bytes& signature) const
{
  auto label_plus = mls_1_0_plus(label);
  const auto content = tls::marshal(SignContent{ label_plus, message });
  auto pub = suite.sig().deserialize(data);
  return suite.sig().verify(content, signature, *pub);
}

SignaturePublicKey
SignaturePublicKey::from_jwk(CipherSuite suite, const std::string& json_str)
{
  auto pub = suite.sig().import_jwk(json_str);
  auto pub_data = suite.sig().serialize(*pub);
  return SignaturePublicKey{ pub_data };
}

std::string
SignaturePublicKey::to_jwk(CipherSuite suite) const
{
  auto pub = suite.sig().deserialize(data);
  return suite.sig().export_jwk(*pub);
}

PublicJWK
PublicJWK::parse(const std::string& jwk_json)
{
  const auto parsed = Signature::parse_jwk(jwk_json);
  const auto scheme = tls_signature_scheme(parsed.sig.id);
  const auto pub_data = parsed.sig.serialize(*parsed.key);
  return { scheme, parsed.key_id, { pub_data } };
}

SignaturePrivateKey
SignaturePrivateKey::generate(CipherSuite suite)
{
  auto priv = suite.sig().generate_key_pair();
  auto priv_data = suite.sig().serialize_private(*priv);
  auto pub = priv->public_key();
  auto pub_data = suite.sig().serialize(*pub);
  return { priv_data, pub_data };
}

SignaturePrivateKey
SignaturePrivateKey::parse(CipherSuite suite, const bytes& data)
{
  auto priv = suite.sig().deserialize_private(data);
  auto pub = priv->public_key();
  auto pub_data = suite.sig().serialize(*pub);
  return { data, pub_data };
}

SignaturePrivateKey
SignaturePrivateKey::derive(CipherSuite suite, const bytes& secret)
{
  auto priv = suite.sig().derive_key_pair(secret);
  auto priv_data = suite.sig().serialize_private(*priv);
  auto pub = priv->public_key();
  auto pub_data = suite.sig().serialize(*pub);
  return { priv_data, pub_data };
}

bytes
SignaturePrivateKey::sign(const CipherSuite& suite,
                          const std::string& label,
                          const bytes& message) const
{
  auto label_plus = mls_1_0_plus(label);
  const auto content = tls::marshal(SignContent{ label_plus, message });
  const auto priv = suite.sig().deserialize_private(data);
  return suite.sig().sign(content, *priv);
}

SignaturePrivateKey::SignaturePrivateKey(bytes priv_data, bytes pub_data)
  : data(std::move(priv_data))
  , public_key{ std::move(pub_data) }
{
}

void
SignaturePrivateKey::set_public_key(CipherSuite suite)
{
  const auto priv = suite.sig().deserialize_private(data);
  auto pub = priv->public_key();
  public_key.data = suite.sig().serialize(*pub);
}

SignaturePrivateKey
SignaturePrivateKey::from_jwk(CipherSuite suite, const std::string& json_str)
{
  auto priv = suite.sig().import_jwk_private(json_str);
  auto priv_data = suite.sig().serialize_private(*priv);
  auto pub = priv->public_key();
  auto pub_data = suite.sig().serialize(*pub);
  return { priv_data, pub_data };
}

std::string
SignaturePrivateKey::to_jwk(CipherSuite suite) const
{
  const auto priv = suite.sig().deserialize_private(data);
  return suite.sig().export_jwk_private(*priv);
}

} // namespace mlspp
