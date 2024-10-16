/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2021 Craig Edwards and D++ contributors
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This folder is a modified fork of libdave, https://github.com/discord/libdave
 * Copyright (c) 2024 Discord, Licensed under MIT
 *
 ************************************************************************************/
#include "parameters.h"

namespace dpp::dave::mls {

::mlspp::CipherSuite::ID ciphersuite_id_for_protocol_version(protocol_version version) noexcept
{
	return ::mlspp::CipherSuite::ID::P256_AES128GCM_SHA256_P256;
}

::mlspp::CipherSuite ciphersuite_for_protocol_version(protocol_version version) noexcept
{
	return ::mlspp::CipherSuite{ciphersuite_id_for_protocol_version(version)};
}

::mlspp::CipherSuite::ID ciphersuite_id_for_signature_version(signature_version version) noexcept
{
	return ::mlspp::CipherSuite::ID::P256_AES128GCM_SHA256_P256;
}

::mlspp::CipherSuite ciphersuite_for_signature_version(signature_version version) noexcept
{
	return ::mlspp::CipherSuite{ciphersuite_id_for_protocol_version(version)};
}

::mlspp::Capabilities leaf_node_capabilities_for_protocol_version(protocol_version version) noexcept
{
	auto capabilities = ::mlspp::Capabilities::create_default();

	capabilities.cipher_suites = {ciphersuite_id_for_protocol_version(version)};
	capabilities.credentials = {::mlspp::CredentialType::basic};

	return capabilities;
}

::mlspp::ExtensionList leaf_node_extensions_for_protocol_version(protocol_version version) noexcept
{
	return ::mlspp::ExtensionList{};
}

::mlspp::ExtensionList group_extensions_for_protocol_version(protocol_version version, const ::mlspp::ExternalSender& external_sender) noexcept
{
	auto extension_list = ::mlspp::ExtensionList{};
	extension_list.add(::mlspp::ExternalSendersExtension{{
	  {external_sender.signature_key, external_sender.credential},
	}});
	return extension_list;
}

} // namespace dpp::dave::mls


