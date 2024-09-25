#include <hpke/certificate.h>
#include <hpke/userinfo_vc.h>
#include <mls/credential.h>
#include <tls/tls_syntax.h>

namespace mlspp {

///
/// X509Credential
///

using mlspp::hpke::Certificate; // NOLINT(misc-unused-using-decls)
using mlspp::hpke::Signature;   // NOLINT(misc-unused-using-decls)
using mlspp::hpke::UserInfoVC;  // NOLINT(misc-unused-using-decls)

static const Signature&
find_signature(Signature::ID id)
{
  switch (id) {
    case Signature::ID::P256_SHA256:
      return Signature::get<Signature::ID::P256_SHA256>();
    case Signature::ID::P384_SHA384:
      return Signature::get<Signature::ID::P384_SHA384>();
    case Signature::ID::P521_SHA512:
      return Signature::get<Signature::ID::P521_SHA512>();
    case Signature::ID::Ed25519:
      return Signature::get<Signature::ID::Ed25519>();
#if !defined(WITH_BORINGSSL)
    case Signature::ID::Ed448:
      return Signature::get<Signature::ID::Ed448>();
#endif
    case Signature::ID::RSA_SHA256:
      return Signature::get<Signature::ID::RSA_SHA256>();
    default:
      throw InvalidParameterError("Unsupported algorithm");
  }
}

static std::vector<X509Credential::CertData>
bytes_to_x509_credential_data(const std::vector<bytes>& data_in)
{
  return stdx::transform<X509Credential::CertData>(
    data_in, [](const bytes& der) { return X509Credential::CertData{ der }; });
}

X509Credential::X509Credential(const std::vector<bytes>& der_chain_in)
  : der_chain(bytes_to_x509_credential_data(der_chain_in))
{
  if (der_chain.empty()) {
    throw std::invalid_argument("empty certificate chain");
  }

  // Parse the chain
  auto parsed = std::vector<Certificate>();
  for (const auto& cert : der_chain) {
    parsed.emplace_back(cert.data);
  }

  // first element represents leaf cert
  const auto& sig = find_signature(parsed[0].public_key_algorithm());
  const auto pub_data = sig.serialize(*parsed[0].public_key);
  _signature_scheme = tls_signature_scheme(parsed[0].public_key_algorithm());
  _public_key = SignaturePublicKey{ pub_data };

  // verify chain for valid signatures
  for (size_t i = 0; i < der_chain.size() - 1; i++) {
    if (!parsed[i].valid_from(parsed[i + 1])) {
      throw std::runtime_error("Certificate Chain validation failure");
    }
  }
}

SignatureScheme
X509Credential::signature_scheme() const
{
  return _signature_scheme;
}

SignaturePublicKey
X509Credential::public_key() const
{
  return _public_key;
}

bool
X509Credential::valid_for(const SignaturePublicKey& pub) const
{
  return pub == public_key();
}

tls::ostream&
operator<<(tls::ostream& str, const X509Credential& obj)
{
  return str << obj.der_chain;
}

tls::istream&
operator>>(tls::istream& str, X509Credential& obj)
{
  auto der_chain = std::vector<X509Credential::CertData>{};
  str >> der_chain;

  auto der_in = stdx::transform<bytes>(
    der_chain, [](const auto& cert_data) { return cert_data.data; });
  obj = X509Credential(der_in);

  return str;
}

bool
operator==(const X509Credential& lhs, const X509Credential& rhs)
{
  return lhs.der_chain == rhs.der_chain;
}

///
/// UserInfoVCCredential
///
UserInfoVCCredential::UserInfoVCCredential(std::string userinfo_vc_jwt_in)
  : userinfo_vc_jwt(std::move(userinfo_vc_jwt_in))
  , _vc(std::make_shared<hpke::UserInfoVC>(userinfo_vc_jwt))
{
}

bool
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
UserInfoVCCredential::valid_for(const SignaturePublicKey& pub) const
{
  const auto& vc_pub = _vc->public_key();
  return pub.data == vc_pub.sig.serialize(*vc_pub.key);
}

bool
UserInfoVCCredential::valid_from(const PublicJWK& pub) const
{
  const auto& sig = _vc->signature_algorithm();
  if (pub.signature_scheme != tls_signature_scheme(sig.id)) {
    return false;
  }

  const auto sig_pub = sig.deserialize(pub.public_key.data);
  return _vc->valid_from(*sig_pub);
}

tls::ostream
operator<<(tls::ostream& str, const UserInfoVCCredential& obj)
{
  return str << from_ascii(obj.userinfo_vc_jwt);
}

tls::istream
operator>>(tls::istream& str, UserInfoVCCredential& obj)
{
  auto jwt = bytes{};
  str >> jwt;
  obj = UserInfoVCCredential(to_ascii(jwt));
  return str;
}

bool
operator==(const UserInfoVCCredential& lhs, const UserInfoVCCredential& rhs)
{
  return lhs.userinfo_vc_jwt == rhs.userinfo_vc_jwt;
}

bool
operator!=(const UserInfoVCCredential& lhs, const UserInfoVCCredential& rhs)
{
  return !(lhs == rhs);
}

///
/// CredentialBinding and MultiCredential
///

struct CredentialBindingTBS
{
  const CipherSuite& cipher_suite;
  const Credential& credential;
  const SignaturePublicKey& credential_key;
  const SignaturePublicKey& signature_key;

  TLS_SERIALIZABLE(cipher_suite, credential, credential_key, signature_key)
};

CredentialBinding::CredentialBinding(CipherSuite cipher_suite_in,
                                     Credential credential_in,
                                     const SignaturePrivateKey& credential_priv,
                                     const SignaturePublicKey& signature_key)
  : cipher_suite(cipher_suite_in)
  , credential(std::move(credential_in))
  , credential_key(credential_priv.public_key)
{
  if (credential.type() == CredentialType::multi_draft_00) {
    throw InvalidParameterError("Multi-credentials cannot be nested");
  }

  if (!credential.valid_for(credential_key)) {
    throw InvalidParameterError("Credential key does not match credential");
  }

  signature = credential_priv.sign(
    cipher_suite, sign_label::multi_credential, to_be_signed(signature_key));
}

bytes
CredentialBinding::to_be_signed(const SignaturePublicKey& signature_key) const
{
  return tls::marshal(CredentialBindingTBS{
    cipher_suite, credential, credential_key, signature_key });
}

bool
CredentialBinding::valid_for(const SignaturePublicKey& signature_key) const
{
  auto valid_self = credential.valid_for(credential_key);
  auto valid_other = credential_key.verify(cipher_suite,
                                           sign_label::multi_credential,
                                           to_be_signed(signature_key),
                                           signature);

  return valid_self && valid_other;
}

MultiCredential::MultiCredential(
  const std::vector<CredentialBindingInput>& binding_inputs,
  const SignaturePublicKey& signature_key)
{
  bindings =
    stdx::transform<CredentialBinding>(binding_inputs, [&](auto&& input) {
      return CredentialBinding(input.cipher_suite,
                               input.credential,
                               input.credential_priv,
                               signature_key);
    });
}

bool
MultiCredential::valid_for(const SignaturePublicKey& pub) const
{
  return stdx::all_of(
    bindings, [&](const auto& binding) { return binding.valid_for(pub); });
}

///
/// Credential
///

CredentialType
Credential::type() const
{
  return tls::variant<CredentialType>::type(_cred);
}

Credential
Credential::basic(const bytes& identity)
{
  return { BasicCredential{ identity } };
}

Credential
Credential::x509(const std::vector<bytes>& der_chain)
{
  return { X509Credential{ der_chain } };
}

Credential
Credential::multi(const std::vector<CredentialBindingInput>& binding_inputs,
                  const SignaturePublicKey& signature_key)
{
  return { MultiCredential{ binding_inputs, signature_key } };
}

Credential
Credential::userinfo_vc(const std::string& userinfo_vc_jwt)
{
  return { UserInfoVCCredential{ userinfo_vc_jwt } };
}

bool
Credential::valid_for(const SignaturePublicKey& pub) const
{
  const auto pub_key_match = overloaded{
    [&](const X509Credential& x509) { return x509.valid_for(pub); },
    [](const BasicCredential& /* basic */) { return true; },
    [&](const UserInfoVCCredential& vc) { return vc.valid_for(pub); },
    [&](const MultiCredential& multi) { return multi.valid_for(pub); },
  };

  return var::visit(pub_key_match, _cred);
}

Credential::Credential(SpecificCredential specific)
  : _cred(std::move(specific))
{
}

} // namespace mlspp
