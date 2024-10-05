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
#include "persisted_key_pair.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <mutex>
#include <functional>
#include <iostream>

#include <bytes/bytes.h>
#include <mls/crypto.h>

#include "parameters.h"
#include "logger.h"

static const std::string SelfSignatureLabel = "DiscordSelfSignature";

static std::string MakeKeyID(const std::string& sessionID, ::mlspp::CipherSuite suite)
{
	return sessionID + "-" + std::to_string((uint16_t)suite.cipher_suite()) + "-" +
	  std::to_string(dpp::dave::mls::KeyVersion);
}

static std::mutex mtx;
static std::map<std::string, std::shared_ptr<::mlspp::SignaturePrivateKey>> map;

namespace dpp::dave::mls {

static std::shared_ptr<::mlspp::SignaturePrivateKey> GetPersistedKeyPair(
  KeyPairContextType ctx,
  const std::string& sessionID,
  ::mlspp::CipherSuite suite)
{
	std::lock_guard lk(mtx);

	std::string id = MakeKeyID(sessionID, suite);

	if (auto it = map.find(id); it != map.end()) {
		return it->second;
	}

	std::shared_ptr<::mlspp::SignaturePrivateKey> ret = ::dpp::dave::mls::detail::GetGenericPersistedKeyPair(ctx, id, suite);

	if (!ret) {
		DISCORD_LOG(LS_ERROR) << "Failed to get key in GetPersistedKeyPair";
		return nullptr;
	}

	map.emplace(id, ret);

	return ret;
}

std::shared_ptr<::mlspp::SignaturePrivateKey> GetPersistedKeyPair(KeyPairContextType ctx,
																  const std::string& sessionID,
																  ProtocolVersion version)
{
	return GetPersistedKeyPair(ctx, sessionID, CiphersuiteForProtocolVersion(version));
}

KeyAndSelfSignature GetPersistedPublicKey(KeyPairContextType ctx,
										  const std::string& sessionID,
										  SignatureVersion version)
{
	auto suite = CiphersuiteForSignatureVersion(version);

	auto pair = GetPersistedKeyPair(ctx, sessionID, suite);

	if (!pair) {
		return {};
	}

	bytes sign_data = from_ascii(sessionID + ":") + pair->public_key.data;

	return {
	  pair->public_key.data.as_vec(),
	  std::move(pair->sign(suite, SelfSignatureLabel, sign_data).as_vec()),
	};
}

bool DeletePersistedKeyPair(KeyPairContextType ctx,
							const std::string& sessionID,
							SignatureVersion version)
{
	std::string id = MakeKeyID(sessionID, CiphersuiteForSignatureVersion(version));

	std::lock_guard lk(mtx);

	map.erase(id);

	return ::dpp::dave::mls::detail::DeleteGenericPersistedKeyPair(ctx, id);
}

} // namespace dpp::dave::mls
