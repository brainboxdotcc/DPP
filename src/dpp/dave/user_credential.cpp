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
#include "user_credential.h"
#include <string>
#include "util.h"

namespace dpp::dave::mls {

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

} // namespace dpp::dave::mls


