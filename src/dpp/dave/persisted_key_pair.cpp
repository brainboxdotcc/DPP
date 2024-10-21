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

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <mutex>
#include <functional>
#include <iostream>
#include <dpp/cluster.h>
#include <bytes/bytes.h>
#include <mls/crypto.h>
#include "parameters.h"

static const std::string self_signature_label = "DiscordSelfSignature";

static std::string make_key_id(const std::string& session_id, ::mlspp::CipherSuite suite) {
	return session_id + "-" + std::to_string((uint16_t)suite.cipher_suite()) + "-" + std::to_string(dpp::dave::mls::KeyVersion);
}

static std::mutex mtx;
static std::map<std::string, std::shared_ptr<::mlspp::SignaturePrivateKey>> map;

namespace dpp::dave::mls {

static std::shared_ptr<::mlspp::SignaturePrivateKey> get_persisted_key_pair(dpp::cluster& creator, key_pair_context_type ctx, const std::string& session_id, ::mlspp::CipherSuite suite) {
	std::lock_guard lk(mtx);

	std::string id = make_key_id(session_id, suite);

	if (auto it = map.find(id); it != map.end()) {
		return it->second;
	}

	std::shared_ptr<::mlspp::SignaturePrivateKey> ret = ::dpp::dave::mls::detail::get_generic_persisted_key_pair(creator, ctx, id, suite);

	if (!ret) {
		creator.log(dpp::ll_warning, "Failed to get key in get_persisted_key_pair");
		return nullptr;
	}

	map.emplace(id, ret);

	return ret;
}

std::shared_ptr<::mlspp::SignaturePrivateKey> get_persisted_key_pair(dpp::cluster& creator, key_pair_context_type ctx, const std::string& session_id, protocol_version version)
{
	return get_persisted_key_pair(creator, ctx, session_id, ciphersuite_for_protocol_version(version));
}

KeyAndSelfSignature get_persisted_public_key(dpp::cluster& creator, key_pair_context_type ctx, const std::string& session_id, signature_version version) {
	auto suite = ciphersuite_for_signature_version(version);
	auto pair = get_persisted_key_pair(creator, ctx, session_id, suite);

	if (!pair) {
		return {};
	}

	bytes sign_data = from_ascii(session_id + ":") + pair->public_key.data;

	return {
		pair->public_key.data.as_vec(),
		std::move(pair->sign(suite, self_signature_label, sign_data).as_vec()),
	};
}

bool delete_persisted_key_pair(dpp::cluster& creator, key_pair_context_type ctx, const std::string& session_id, signature_version version) {
	std::string id = make_key_id(session_id, ciphersuite_for_signature_version(version));
	std::lock_guard lk(mtx);
	map.erase(id);
	return ::dpp::dave::mls::detail::delete_generic_persisted_key_pair(creator, ctx, id);
}

}
