#pragma once

#include <mls/core_types.h>
#include <mls/crypto.h>
#include <mls/messages.h>

#include "version.h"

namespace discord {
namespace dave {
namespace mls {

::mlspp::CipherSuite::ID CiphersuiteIDForProtocolVersion(ProtocolVersion version) noexcept;
::mlspp::CipherSuite CiphersuiteForProtocolVersion(ProtocolVersion version) noexcept;
::mlspp::CipherSuite::ID CiphersuiteIDForSignatureVersion(SignatureVersion version) noexcept;
::mlspp::CipherSuite CiphersuiteForSignatureVersion(SignatureVersion version) noexcept;
::mlspp::Capabilities LeafNodeCapabilitiesForProtocolVersion(ProtocolVersion version) noexcept;
::mlspp::ExtensionList LeafNodeExtensionsForProtocolVersion(ProtocolVersion version) noexcept;
::mlspp::ExtensionList GroupExtensionsForProtocolVersion(
  ProtocolVersion version,
  const ::mlspp::ExternalSender& externalSender) noexcept;

} // namespace mls
} // namespace dave
} // namespace discord
