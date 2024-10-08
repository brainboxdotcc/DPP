#include "group.h"
#include "openssl_common.h"
#include "rsa.h"
#include <hpke/certificate.h>
#include <hpke/signature.h>
#include <memory>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <tls/compat.h>

namespace mlspp::hpke {
///
/// Utility functions
///

static std::optional<bytes>
asn1_octet_string_to_bytes(const ASN1_OCTET_STRING* octets)
{
  if (octets == nullptr) {
    return std::nullopt;
  }
  const auto* ptr = ASN1_STRING_get0_data(octets);
  const auto len = ASN1_STRING_length(octets);
  // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
  return std::vector<uint8_t>(ptr, ptr + len);
}

static std::string
asn1_string_to_std_string(const ASN1_STRING* asn1_string)
{
  const auto* data = ASN1_STRING_get0_data(asn1_string);
  const auto data_size = static_cast<size_t>(ASN1_STRING_length(asn1_string));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto str = std::string(reinterpret_cast<const char*>(data));
  if (str.size() != data_size) {
    throw std::runtime_error("Malformed ASN.1 string");
  }
  return str;
}

static std::chrono::system_clock::time_point
asn1_time_to_chrono(const ASN1_TIME* asn1_time)
{
  auto epoch_chrono = std::chrono::system_clock::time_point();
  auto epoch_time_t = std::chrono::system_clock::to_time_t(epoch_chrono);
  auto epoch_asn1 = make_typed_unique(ASN1_TIME_set(nullptr, epoch_time_t));
  if (!epoch_asn1) {
    throw openssl_error();
  }

  auto secs = int(0);
  auto days = int(0);
  if (ASN1_TIME_diff(&days, &secs, epoch_asn1.get(), asn1_time) != 1) {
    throw openssl_error();
  }

  auto delta = std::chrono::seconds(secs) + std::chrono::hours(24 * days);
  return std::chrono::system_clock::time_point(delta);
}

///
/// ParsedCertificate
///

const int Certificate::NameType::organization = NID_organizationName;
const int Certificate::NameType::common_name = NID_commonName;
const int Certificate::NameType::organizational_unit =
  NID_organizationalUnitName;
const int Certificate::NameType::country = NID_countryName;
const int Certificate::NameType::serial_number = NID_serialNumber;
const int Certificate::NameType::state_or_province_name =
  NID_stateOrProvinceName;

struct RFC822Name
{
  std::string value;
};

struct DNSName
{
  std::string value;
};

using GeneralName = tls::var::variant<RFC822Name, DNSName>;

struct Certificate::ParsedCertificate
{

  static std::unique_ptr<ParsedCertificate> parse(const bytes& der)
  {
    const auto* buf = der.data();
    auto cert =
      make_typed_unique(d2i_X509(nullptr, &buf, static_cast<int>(der.size())));
    if (cert == nullptr) {
      throw openssl_error();
    }

    return std::make_unique<ParsedCertificate>(cert.release());
  }

  static bytes compute_digest(const X509* cert)
  {
    const auto* md = EVP_sha256();
    auto digest = bytes(EVP_MD_size(md));
    unsigned int out_size = 0;
    if (1 != X509_digest(cert, md, digest.data(), &out_size)) {
      throw openssl_error();
    }
    return digest;
  }

  // Note: This method does not implement total general name parsing.
  // Duplicate entries are not supported; if they are present, the last one
  // presented by OpenSSL is chosen.
  static ParsedName parse_names(const X509_NAME* x509_name)
  {
    if (x509_name == nullptr) {
      throw openssl_error();
    }

    ParsedName parsed_name;

    for (int i = X509_NAME_entry_count(x509_name) - 1; i >= 0; i--) {
      auto* entry = X509_NAME_get_entry(x509_name, i);
      if (entry == nullptr) {
        continue;
      }

      auto* oid = X509_NAME_ENTRY_get_object(entry);
      auto* asn_str = X509_NAME_ENTRY_get_data(entry);
      if (oid == nullptr || asn_str == nullptr) {
        continue;
      }

      const int nid = OBJ_obj2nid(oid);
      const std::string parsed_value = asn1_string_to_std_string(asn_str);
      parsed_name[nid] = parsed_value;
    }

    return parsed_name;
  }

  // Parse Subject Key Identifier Extension
  static std::optional<bytes> parse_skid(X509* cert)
  {
    return asn1_octet_string_to_bytes(X509_get0_subject_key_id(cert));
  }

  // Parse Authority Key Identifier
  static std::optional<bytes> parse_akid(X509* cert)
  {
    return asn1_octet_string_to_bytes(X509_get0_authority_key_id(cert));
  }

  static std::vector<GeneralName> parse_san(X509* cert)
  {
    std::vector<GeneralName> names;

#ifdef WITH_BORINGSSL
    using san_names_nb_t = size_t;
#else
    using san_names_nb_t = int;
#endif

    san_names_nb_t san_names_nb = 0;

    auto* ext_ptr =
      X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* san_ptr = reinterpret_cast<STACK_OF(GENERAL_NAME)*>(ext_ptr);
    const auto san_names = make_typed_unique(san_ptr);
    san_names_nb = sk_GENERAL_NAME_num(san_names.get());

    // Check each name within the extension
    for (san_names_nb_t i = 0; i < san_names_nb; i++) {
      auto* current_name = sk_GENERAL_NAME_value(san_names.get(), i);
      if (current_name->type == GEN_DNS) {
        const auto dns_name =
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
          asn1_string_to_std_string(current_name->d.dNSName);
        names.emplace_back(DNSName{ dns_name });
      } else if (current_name->type == GEN_EMAIL) {
        const auto email =
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access
          asn1_string_to_std_string(current_name->d.rfc822Name);
        names.emplace_back(RFC822Name{ email });
      }
    }

    return names;
  }

  explicit ParsedCertificate(X509* x509_in)
    : x509(x509_in, typed_delete<X509>)
    , pub_key_id(public_key_algorithm(x509.get()))
    , sig_algo(signature_algorithm(x509.get()))
    , issuer_hash(X509_issuer_name_hash(x509.get()))
    , subject_hash(X509_subject_name_hash(x509.get()))
    , issuer(parse_names(X509_get_issuer_name(x509.get())))
    , subject(parse_names(X509_get_subject_name(x509.get())))
    , subject_key_id(parse_skid(x509.get()))
    , authority_key_id(parse_akid(x509.get()))
    , sub_alt_names(parse_san(x509.get()))
    , is_ca(X509_check_ca(x509.get()) != 0)
    , hash(compute_digest(x509.get()))
    , not_before(asn1_time_to_chrono(X509_get0_notBefore(x509.get())))
    , not_after(asn1_time_to_chrono(X509_get0_notAfter(x509.get())))
  {
  }

  ParsedCertificate(const ParsedCertificate& other)
    : x509(nullptr, typed_delete<X509>)
    , pub_key_id(public_key_algorithm(other.x509.get()))
    , sig_algo(signature_algorithm(other.x509.get()))
    , issuer_hash(other.issuer_hash)
    , subject_hash(other.subject_hash)
    , issuer(other.issuer)
    , subject(other.subject)
    , subject_key_id(other.subject_key_id)
    , authority_key_id(other.authority_key_id)
    , sub_alt_names(other.sub_alt_names)
    , is_ca(other.is_ca)
    , hash(other.hash)
    , not_before(other.not_before)
    , not_after(other.not_after)
  {
    if (1 != X509_up_ref(other.x509.get())) {
      throw openssl_error();
    }
    x509.reset(other.x509.get());
  }

  static Signature::ID public_key_algorithm(X509* x509)
  {
#if WITH_BORINGSSL
    const auto pub = make_typed_unique(X509_get_pubkey(x509));
    const auto* pub_ptr = pub.get();
#else
    const auto* pub_ptr = X509_get0_pubkey(x509);
#endif

    switch (EVP_PKEY_base_id(pub_ptr)) {
      case EVP_PKEY_ED25519:
        return Signature::ID::Ed25519;
#if !defined(WITH_BORINGSSL)
      case EVP_PKEY_ED448:
        return Signature::ID::Ed448;
#endif
      case EVP_PKEY_EC: {
        auto key_size = EVP_PKEY_bits(pub_ptr);
        switch (key_size) {
          case 256:
            return Signature::ID::P256_SHA256;
          case 384:
            return Signature::ID::P384_SHA384;
          case 521:
            return Signature::ID::P521_SHA512;
          default:
            throw std::runtime_error("Unknown curve");
        }
      }
      case EVP_PKEY_RSA:
        // RSA public keys are not specific to an algorithm
        return Signature::ID::RSA_SHA256;
      default:
        break;
    }
    throw std::runtime_error("Unsupported public key algorithm");
  }

  static Signature::ID signature_algorithm(X509* cert)
  {
    auto nid = X509_get_signature_nid(cert);
    switch (nid) {
      case EVP_PKEY_ED25519:
        return Signature::ID::Ed25519;
#if !defined(WITH_BORINGSSL)
      case EVP_PKEY_ED448:
        return Signature::ID::Ed448;
#endif
      case NID_ecdsa_with_SHA256:
        return Signature::ID::P256_SHA256;
      case NID_ecdsa_with_SHA384:
        return Signature::ID::P384_SHA384;
      case NID_ecdsa_with_SHA512:
        return Signature::ID::P521_SHA512;
      case NID_sha1WithRSAEncryption:
        // We fall through to SHA256 for SHA1 because we do not implement SHA-1.
      case NID_sha256WithRSAEncryption:
        return Signature::ID::RSA_SHA256;
      case NID_sha384WithRSAEncryption:
        return Signature::ID::RSA_SHA384;
      case NID_sha512WithRSAEncryption:
        return Signature::ID::RSA_SHA512;
      default:
        break;
    }

    throw std::runtime_error("Unsupported signature algorithm");
  }

  typed_unique_ptr<EVP_PKEY> public_key() const
  {
    return make_typed_unique<EVP_PKEY>(X509_get_pubkey(x509.get()));
  }

  Certificate::ExpirationStatus expiration_status() const
  {
    auto now = std::chrono::system_clock::now();

    if (now < not_before) {
      return Certificate::ExpirationStatus::inactive;
    }

    if (now > not_after) {
      return Certificate::ExpirationStatus::expired;
    }

    return Certificate::ExpirationStatus::active;
  }

  bytes raw() const
  {
    auto out = bytes(i2d_X509(x509.get(), nullptr));
    auto* ptr = out.data();
    i2d_X509(x509.get(), &ptr);
    return out;
  }

  typed_unique_ptr<X509> x509;
  const Signature::ID pub_key_id;
  const Signature::ID sig_algo;
  const uint64_t issuer_hash;
  const uint64_t subject_hash;
  const ParsedName issuer;
  const ParsedName subject;
  const std::optional<bytes> subject_key_id;
  const std::optional<bytes> authority_key_id;
  const std::vector<GeneralName> sub_alt_names;
  const bool is_ca;
  const bytes hash;
  const std::chrono::system_clock::time_point not_before;
  const std::chrono::system_clock::time_point not_after;
};

///
/// Certificate
///

static std::unique_ptr<Signature::PublicKey>
signature_key(EVP_PKEY* pkey)
{
  switch (EVP_PKEY_base_id(pkey)) {
    case EVP_PKEY_RSA:
      return std::make_unique<RSASignature::PublicKey>(pkey);

    case EVP_PKEY_ED448:
    case EVP_PKEY_ED25519:
    case EVP_PKEY_EC:
      return std::make_unique<EVPGroup::PublicKey>(pkey);

    default:
      throw std::runtime_error("Unsupported algorithm");
  }
}

Certificate::Certificate(std::unique_ptr<ParsedCertificate>&& parsed_cert_in)
  : parsed_cert(std::move(parsed_cert_in))
  , public_key(signature_key(parsed_cert->public_key().release()))
  , raw(parsed_cert->raw())
{
}

Certificate::Certificate(const bytes& der)
  : parsed_cert(ParsedCertificate::parse(der))
  , public_key(signature_key(parsed_cert->public_key().release()))
  , raw(der)
{
}

Certificate::Certificate(const Certificate& other)
  : parsed_cert(std::make_unique<ParsedCertificate>(*other.parsed_cert))
  , public_key(signature_key(parsed_cert->public_key().release()))
  , raw(other.raw)
{
}

Certificate::~Certificate() = default;

std::vector<Certificate>
Certificate::parse_pem(const bytes& pem)
{
  auto size_int = static_cast<int>(pem.size());
  auto bio = make_typed_unique<BIO>(BIO_new_mem_buf(pem.data(), size_int));
  if (!bio) {
    throw openssl_error();
  }

  auto certs = std::vector<Certificate>();
  while (true) {
    auto x509 = make_typed_unique<X509>(
      PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
    if (!x509) {
      // NOLINTNEXTLINE(hicpp-signed-bitwise)
      auto err = ERR_GET_REASON(ERR_peek_last_error());
      if (err == PEM_R_NO_START_LINE) {
        // No more objects to read
        break;
      }

      throw openssl_error();
    }

    auto parsed = std::make_unique<ParsedCertificate>(x509.release());
    certs.emplace_back(std::move(parsed));
  }

  return certs;
}

bool
Certificate::valid_from(const Certificate& parent) const
{
  auto pub = parent.parsed_cert->public_key();
  return (1 == X509_verify(parsed_cert->x509.get(), pub.get()));
}

uint64_t
Certificate::issuer_hash() const
{
  return parsed_cert->issuer_hash;
}

uint64_t
Certificate::subject_hash() const
{
  return parsed_cert->subject_hash;
}

Certificate::ParsedName
Certificate::subject() const
{
  return parsed_cert->subject;
}

Certificate::ParsedName
Certificate::issuer() const
{
  return parsed_cert->issuer;
}

bool
Certificate::is_ca() const
{
  return parsed_cert->is_ca;
}

Certificate::ExpirationStatus
Certificate::expiration_status() const
{
  return parsed_cert->expiration_status();
}

std::optional<bytes>
Certificate::subject_key_id() const
{
  return parsed_cert->subject_key_id;
}

std::optional<bytes>
Certificate::authority_key_id() const
{
  return parsed_cert->authority_key_id;
}

std::vector<std::string>
Certificate::email_addresses() const
{
  std::vector<std::string> emails;
  for (const auto& name : parsed_cert->sub_alt_names) {
    if (tls::var::holds_alternative<RFC822Name>(name)) {
      emails.emplace_back(tls::var::get<RFC822Name>(name).value);
    }
  }
  return emails;
}

std::vector<std::string>
Certificate::dns_names() const
{
  std::vector<std::string> domains;
  for (const auto& name : parsed_cert->sub_alt_names) {
    if (tls::var::holds_alternative<DNSName>(name)) {
      domains.emplace_back(tls::var::get<DNSName>(name).value);
    }
  }

  return domains;
}

bytes
Certificate::hash() const
{
  return parsed_cert->hash;
}

std::chrono::system_clock::time_point
Certificate::not_before() const
{
  return parsed_cert->not_before;
}

std::chrono::system_clock::time_point
Certificate::not_after() const
{
  return parsed_cert->not_after;
}

Signature::ID
Certificate::public_key_algorithm() const
{
  return parsed_cert->pub_key_id;
}

Signature::ID
Certificate::signature_algorithm() const
{
  return parsed_cert->sig_algo;
}

bool
operator==(const Certificate& lhs, const Certificate& rhs)
{
  return lhs.raw == rhs.raw;
}

} // namespace mlspp::hpke
