#include "user_credential.h"
#include <string>
#include "util.h"

namespace discord {
namespace dave {
namespace mls {

::mlspp::Credential CreateUserCredential(const std::string& userId, ProtocolVersion version)
{
    // convert the string user ID to a big endian uint64_t
    auto userID = std::stoull(userId);
    auto credentialBytes = BigEndianBytesFrom(userID);

    return ::mlspp::Credential::basic(credentialBytes);
}

std::string UserCredentialToString(const ::mlspp::Credential& cred, ProtocolVersion version)
{
    if (cred.type() != ::mlspp::CredentialType::basic) {
        return "";
    }

    const auto& basic = cred.template get<::mlspp::BasicCredential>();

    auto uidVal = FromBigEndianBytes(basic.identity);

    return std::to_string(uidVal);
}

} // namespace mls
} // namespace dave
} // namespace discord
