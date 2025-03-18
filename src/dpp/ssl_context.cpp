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
 ************************************************************************************/
#include <dpp/ssl_context.h>
#include <dpp/exception.h>
#include <vector>
#include <memory>
#include <openssl/ssl.h>
#include <mutex>
#include <shared_mutex>
#include <dpp/wrapped_ssl_ctx.h>

namespace dpp::detail {

/**
 * @brief The vector of pairs of wrapped contexts is efficient for small numbers of contexts.
 * In a real world production application we expect to have 2 to 5 at most contexts, and for
 * most bots that do not use server ports, there will be only one context on port 0.
 * This is O(n), but in the most common situation of having one entry, it is O(1).
 */
static std::vector<std::pair<uint16_t, std::unique_ptr<wrapped_ssl_ctx>>> contexts;

/**
 * @brief Managing SSL contexts is thread-safe.
 */
static std::shared_mutex context_mutex;

void release_ssl_context(uint16_t port) {
	std::unique_lock lock(context_mutex);
	auto it = std::remove_if(contexts.begin(), contexts.end(), [port](const auto& entry) { return entry.first == port; });
	if (it != contexts.end()) {
		contexts.erase(it, contexts.end());
	}
}

wrapped_ssl_ctx* generate_ssl_context(uint16_t port, const std::string &private_key, const std::string &public_key) {
	{
		std::shared_lock lock(context_mutex);
		for (const auto& [p, ctx] : contexts) {
			if (p == port) {
				return ctx.get();
			}
		}
	}

	std::unique_ptr<wrapped_ssl_ctx> context = std::make_unique<wrapped_ssl_ctx>(port != 0);

	if (port != 0) {
		if (SSL_CTX_use_certificate_file(context->context, public_key.c_str(), SSL_FILETYPE_PEM) <= 0) {
			throw dpp::connection_exception(err_ssl_context, "Failed to set public key certificate");
		}
		if (SSL_CTX_use_PrivateKey_file(context->context, private_key.c_str(), SSL_FILETYPE_PEM) <= 0) {
			throw dpp::connection_exception(err_ssl_context, "Failed to set private key certificate");
		}
	}

	/* This sets the allowed SSL/TLS versions for the connection.
	 * Do not allow SSL 3.0, TLS 1.0 or 1.1
	 * https://www.packetlabs.net/posts/tls-1-1-no-longer-secure/
	 */
	if (!SSL_CTX_set_min_proto_version(context->context, TLS1_2_VERSION)) {
		throw dpp::connection_exception(err_ssl_version, "Failed to set minimum SSL version!");
	}

	std::unique_lock lock(context_mutex);
	contexts.emplace_back(port, std::move(context));
	return contexts.back().second.get();
}

}
