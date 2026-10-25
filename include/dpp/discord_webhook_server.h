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
#pragma once

#include <dpp/export.h>
#include <dpp/http_server.h>
#include <dpp/signature_verifier.h>

namespace dpp {

/**
 * @brief Creates a HTTP server which listens for incoming
 * Discord interactions, and if verified as valid, raises them
 * as cluster events, returning the response back.
 * Note that Discord requires all interaction endpoints to
 * have a valid SSL certificate (not self signed) so in most
 * cases you should put this port behind a reverse proxy, e.g.
 * nginx, apache, etc.
 */
struct discord_webhook_server : public http_server {

	/**
	 * @brief Verifier for signed requests
	 */
	signature_verifier verifier;

	/**
	 * @brief Public key from application dashboard
	 */
	std::string public_key_hex;

	/**
	 * @brief Constructor for creation of a HTTP(S) server
	 * @param creator Cluster creator
	 * @param discord_public_key Public key for the application from the application dashboard page
	 * @param address address to bind to, use "0.0.0.0" to bind to all local addresses
	 * @param port port to bind to. You should generally use a port > 1024.
	 * @param ssl_private_key Private key PEM file for HTTPS/SSL. If empty, a plaintext server is created
	 * @param ssl_public_key Public key PEM file for HTTPS/SSL. If empty, a plaintext server is created
	 */
	discord_webhook_server(cluster* creator, const std::string& discord_public_key, const std::string_view address, uint16_t port,  const std::string& ssl_private_key = "", const std::string& ssl_public_key = "");

	/**
	 * @brief Handle Discord outbound webhook
	 * @param request Request from discord
	 */
	void handle_request(http_server_request* request);

	/**
	 * @brief Virtual dtor
	 */
	virtual ~discord_webhook_server() = default;
};

}
