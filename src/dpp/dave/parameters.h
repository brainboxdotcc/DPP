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
#pragma once

#include <mls/core_types.h>
#include <mls/crypto.h>
#include <mls/messages.h>

#include "version.h"

namespace dpp::dave::mls {

/**
 * @brief Get ciphersuite id for protocol version
 * @param version protocol version
 * @return ciphersuite id
 */
::mlspp::CipherSuite::ID ciphersuite_id_for_protocol_version(protocol_version version) noexcept;

/**
 * @brief Get ciphersuite for protocol version
 * @param version protocol version
 * @return ciphersuite
 */
::mlspp::CipherSuite ciphersuite_for_protocol_version(protocol_version version) noexcept;

/**
 * @brief Get ciphersuite id for signature version
 * @param version signature version
 * @return Ciphersuite id
 */
::mlspp::CipherSuite::ID ciphersuite_id_for_signature_version(signature_version version) noexcept;

/**
 * @brief Get ciphersuite for singnature version
 * @param version signature version
 * @return Ciphersuite
 */
::mlspp::CipherSuite ciphersuite_for_signature_version(signature_version version) noexcept;

/**
 * @brief Get leaf node capabilities for protocol version
 * @param version protocol version
 * @return capabilities
 */
::mlspp::Capabilities leaf_node_capabilities_for_protocol_version(protocol_version version) noexcept;

/**
 * @brief Get leaf node extensions for protocol version
 * @param version protocol version
 * @return extension list
 */
::mlspp::ExtensionList leaf_node_extensions_for_protocol_version(protocol_version version) noexcept;

/**
 * @brief Get group extensions for protocol version
 * @param version protocol bersion
 * @param external_sender external sender
 * @return extension list
 */
::mlspp::ExtensionList group_extensions_for_protocol_version(protocol_version version, const ::mlspp::ExternalSender& external_sender) noexcept;

}
