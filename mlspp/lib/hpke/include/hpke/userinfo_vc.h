#pragma once
#include <memory>
#include <optional>

#include <bytes/bytes.h>
#include <chrono>
#include <hpke/signature.h>
#include <map>

using namespace mlspp::bytes_ns;

namespace mlspp::hpke {

struct UserInfoClaimsAddress
{
  std::optional<std::string> formatted;
  std::optional<std::string> street_address;
  std::optional<std::string> locality;
  std::optional<std::string> region;
  std::optional<std::string> postal_code;
  std::optional<std::string> country;
};

struct UserInfoClaims
{

  std::optional<std::string> sub;
  std::optional<std::string> name;
  std::optional<std::string> given_name;
  std::optional<std::string> family_name;
  std::optional<std::string> middle_name;
  std::optional<std::string> nickname;
  std::optional<std::string> preferred_username;
  std::optional<std::string> profile;
  std::optional<std::string> picture;
  std::optional<std::string> website;
  std::optional<std::string> email;
  std::optional<bool> email_verified;
  std::optional<std::string> gender;
  std::optional<std::string> birthdate;
  std::optional<std::string> zoneinfo;
  std::optional<std::string> locale;
  std::optional<std::string> phone_number;
  std::optional<bool> phone_number_verified;
  std::optional<UserInfoClaimsAddress> address;
  std::optional<uint64_t> updated_at;

  static UserInfoClaims from_json(const std::string& cred_subject);
};

struct UserInfoVC
{
private:
  struct ParsedCredential;
  std::shared_ptr<ParsedCredential> parsed_cred;

public:
  explicit UserInfoVC(std::string jwt);
  UserInfoVC() = default;
  UserInfoVC(const UserInfoVC& other) = default;
  ~UserInfoVC() = default;
  UserInfoVC& operator=(const UserInfoVC& other) = default;
  UserInfoVC& operator=(UserInfoVC&& other) = default;

  const Signature& signature_algorithm() const;
  std::string issuer() const;
  std::optional<std::string> key_id() const;
  std::chrono::system_clock::time_point not_before() const;
  std::chrono::system_clock::time_point not_after() const;
  const std::string& raw_credential() const;
  const UserInfoClaims& subject() const;
  const Signature::PublicJWK& public_key() const;

  bool valid_from(const Signature::PublicKey& issuer_key) const;

  std::string raw;
};

bool
operator==(const UserInfoVC& lhs, const UserInfoVC& rhs);

} // namespace mlspp::hpke
