#pragma once
#include <memory>
#include <optional>

#include <bytes/bytes.h>
#include <chrono>
#include <hpke/signature.h>
#include <map>

using namespace mlspp::bytes_ns;

namespace mlspp::hpke {

struct Certificate
{
private:
  struct ParsedCertificate;
  std::unique_ptr<ParsedCertificate> parsed_cert;

public:
  struct NameType
  {
    static const int organization;
    static const int common_name;
    static const int organizational_unit;
    static const int country;
    static const int serial_number;
    static const int state_or_province_name;
  };

  using ParsedName = std::map<int, std::string>;

  // Certificate Expiration Status
  enum struct ExpirationStatus
  {
    inactive, // now < notBefore
    active,   // notBefore < now < notAfter
    expired,  // notAfter < now
  };

  explicit Certificate(const bytes& der);
  explicit Certificate(std::unique_ptr<ParsedCertificate>&& parsed_cert_in);
  Certificate() = delete;
  Certificate(const Certificate& other);
  ~Certificate();

  static std::vector<Certificate> parse_pem(const bytes& pem);

  bool valid_from(const Certificate& parent) const;

  // Accessors for parsed certificate elements
  uint64_t issuer_hash() const;
  uint64_t subject_hash() const;
  ParsedName issuer() const;
  ParsedName subject() const;
  bool is_ca() const;
  ExpirationStatus expiration_status() const;
  std::optional<bytes> subject_key_id() const;
  std::optional<bytes> authority_key_id() const;
  std::vector<std::string> email_addresses() const;
  std::vector<std::string> dns_names() const;
  bytes hash() const;
  std::chrono::system_clock::time_point not_before() const;
  std::chrono::system_clock::time_point not_after() const;
  Signature::ID public_key_algorithm() const;
  Signature::ID signature_algorithm() const;

  const std::unique_ptr<Signature::PublicKey> public_key;
  const bytes raw;
};

bool
operator==(const Certificate& lhs, const Certificate& rhs);

} // namespace mlspp::hpke
