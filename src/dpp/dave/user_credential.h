#pragma once

#include <string>
#include <mls/credential.h>
#include "version.h"

namespace discord {
namespace dave {
namespace mls {

::mlspp::Credential CreateUserCredential(const std::string& userId, ProtocolVersion version);
std::string UserCredentialToString(const ::mlspp::Credential& cred, ProtocolVersion version);

} // namespace mls
} // namespace dave
} // namespace discord
