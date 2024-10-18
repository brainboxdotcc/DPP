#pragma once

#include <mls/common.h>
#include <mls/crypto.h>

namespace mlspp {

namespace hpke {
struct UserInfoVC;
}

// struct {
//     opaque identity<0..2^16-1>;
//     SignaturePublicKey public_key;
// } BasicCredential;
struct BasicCredential
{
  BasicCredential() {}

  BasicCredential(bytes identity_in)
    : identity(std::move(identity_in))
  {
  }

  bytes identity;

  TLS_SERIALIZABLE(identity)
};

struct X509Credential
{
  struct CertData
  {
    bytes data;

    TLS_SERIALIZABLE(data)
  };

  X509Credential() = default;
  explicit X509Credential(const std::vector<bytes>& der_chain_in);

  SignatureScheme signature_scheme() const;
  SignaturePublicKey public_key() const;
  bool valid_for(const SignaturePublicKey& pub) const;

  // TODO(rlb) This should be const or exposed via a method
  std::vector<CertData> der_chain;

private:
  SignaturePublicKey _public_key;
  SignatureScheme _signature_scheme;
};

tls::ostream&
operator<<(tls::ostream& str, const X509Credential& obj);

tls::istream&
operator>>(tls::istream& str, X509Credential& obj);

struct UserInfoVCCredential
{
  UserInfoVCCredential() = default;
  explicit UserInfoVCCredential(std::string userinfo_vc_jwt_in);

  std::string userinfo_vc_jwt;

  bool valid_for(const SignaturePublicKey& pub) const;
  bool valid_from(const PublicJWK& pub) const;

  friend tls::ostream operator<<(tls::ostream& str,
                                 const UserInfoVCCredential& obj);
  friend tls::istream operator>>(tls::istream& str, UserInfoVCCredential& obj);
  friend bool operator==(const UserInfoVCCredential& lhs,
                         const UserInfoVCCredential& rhs);
  friend bool operator!=(const UserInfoVCCredential& lhs,
                         const UserInfoVCCredential& rhs);

private:
  std::shared_ptr<hpke::UserInfoVC> _vc;
};

bool
operator==(const X509Credential& lhs, const X509Credential& rhs);

enum struct CredentialType : uint16_t
{
  reserved = 0,
  basic = 1,
  x509 = 2,

  userinfo_vc_draft_00 = 0xFE00,
  multi_draft_00 = 0xFF00,

  // GREASE values, included here mainly so that debugger output looks nice
  GREASE_0 = 0x0A0A,
  GREASE_1 = 0x1A1A,
  GREASE_2 = 0x2A2A,
  GREASE_3 = 0x3A3A,
  GREASE_4 = 0x4A4A,
  GREASE_5 = 0x5A5A,
  GREASE_6 = 0x6A6A,
  GREASE_7 = 0x7A7A,
  GREASE_8 = 0x8A8A,
  GREASE_9 = 0x9A9A,
  GREASE_A = 0xAAAA,
  GREASE_B = 0xBABA,
  GREASE_C = 0xCACA,
  GREASE_D = 0xDADA,
  GREASE_E = 0xEAEA,
};

// struct {
//   Credential credential;
//   SignaturePublicKey credential_key;
//   opaque signature<V>;
// } CredentialBinding
//
// struct {
//   CredentialBinding bindings<V>;
// } MultiCredential;
struct CredentialBinding;
struct CredentialBindingInput;

struct MultiCredential
{
  MultiCredential() = default;
  MultiCredential(const std::vector<CredentialBindingInput>& binding_inputs,
                  const SignaturePublicKey& signature_key);

  std::vector<CredentialBinding> bindings;

  bool valid_for(const SignaturePublicKey& pub) const;

  TLS_SERIALIZABLE(bindings)
};

// struct {
//     CredentialType credential_type;
//     select (credential_type) {
//         case basic:
//             BasicCredential;
//
//         case x509:
//             opaque cert_data<1..2^24-1>;
//     };
// } Credential;
struct Credential
{
  Credential() = default;

  CredentialType type() const;

  template<typename T>
  const T& get() const
  {
    return var::get<T>(_cred);
  }

  static Credential basic(const bytes& identity);
  static Credential x509(const std::vector<bytes>& der_chain);
  static Credential userinfo_vc(const std::string& userinfo_vc_jwt);
  static Credential multi(
    const std::vector<CredentialBindingInput>& binding_inputs,
    const SignaturePublicKey& signature_key);

  bool valid_for(const SignaturePublicKey& pub) const;

  TLS_SERIALIZABLE(_cred)
  TLS_TRAITS(tls::variant<CredentialType>)

private:
  using SpecificCredential = var::variant<BasicCredential,
                                          X509Credential,
                                          UserInfoVCCredential,
                                          MultiCredential>;

  Credential(SpecificCredential specific);
  SpecificCredential _cred;
};

// XXX(RLB): This struct needs to appear below Credential so that all types are
// concrete at the appropriate points.
struct CredentialBindingInput
{
  CipherSuite cipher_suite;
  Credential credential;
  const SignaturePrivateKey& credential_priv;
};

struct CredentialBinding
{
  CipherSuite cipher_suite;
  Credential credential;
  SignaturePublicKey credential_key;
  bytes signature;

  CredentialBinding() = default;
  CredentialBinding(CipherSuite suite_in,
                    Credential credential_in,
                    const SignaturePrivateKey& credential_priv,
                    const SignaturePublicKey& signature_key);

  bool valid_for(const SignaturePublicKey& signature_key) const;

  TLS_SERIALIZABLE(cipher_suite, credential, credential_key, signature)

private:
  bytes to_be_signed(const SignaturePublicKey& signature_key) const;
};

} // namespace mlspp

namespace mlspp::tls {

TLS_VARIANT_MAP(mlspp::CredentialType,
                mlspp::BasicCredential,
                basic)
TLS_VARIANT_MAP(mlspp::CredentialType,
                mlspp::X509Credential,
                x509)
TLS_VARIANT_MAP(mlspp::CredentialType,
                mlspp::UserInfoVCCredential,
                userinfo_vc_draft_00)
TLS_VARIANT_MAP(mlspp::CredentialType,
                mlspp::MultiCredential,
                multi_draft_00)

} // namespace mlspp::tls
