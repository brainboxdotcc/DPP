#include "parameters.h"

namespace discord {
namespace dave {
namespace mls {

::mlspp::CipherSuite::ID CiphersuiteIDForProtocolVersion(ProtocolVersion version) noexcept
{
    return ::mlspp::CipherSuite::ID::P256_AES128GCM_SHA256_P256;
}

::mlspp::CipherSuite CiphersuiteForProtocolVersion(ProtocolVersion version) noexcept
{
    return ::mlspp::CipherSuite{CiphersuiteIDForProtocolVersion(version)};
}

::mlspp::CipherSuite::ID CiphersuiteIDForSignatureVersion(SignatureVersion version) noexcept
{
    return ::mlspp::CipherSuite::ID::P256_AES128GCM_SHA256_P256;
}

::mlspp::CipherSuite CiphersuiteForSignatureVersion(SignatureVersion version) noexcept
{
    return ::mlspp::CipherSuite{CiphersuiteIDForProtocolVersion(version)};
}

::mlspp::Capabilities LeafNodeCapabilitiesForProtocolVersion(ProtocolVersion version) noexcept
{
    auto capabilities = ::mlspp::Capabilities::create_default();

    capabilities.cipher_suites = {CiphersuiteIDForProtocolVersion(version)};
    capabilities.credentials = {::mlspp::CredentialType::basic};

    return capabilities;
}

::mlspp::ExtensionList LeafNodeExtensionsForProtocolVersion(ProtocolVersion version) noexcept
{
    return ::mlspp::ExtensionList{};
}

::mlspp::ExtensionList GroupExtensionsForProtocolVersion(
  ProtocolVersion version,
  const ::mlspp::ExternalSender& externalSender) noexcept
{
    auto extensionList = ::mlspp::ExtensionList{};

    extensionList.add(::mlspp::ExternalSendersExtension{{
      {externalSender.signature_key, externalSender.credential},
    }});

    return extensionList;
}

} // namespace mls
} // namespace dave
} // namespace discord
