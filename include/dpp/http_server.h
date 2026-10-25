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
#include <dpp/cluster.h>
#include <dpp/socket_listener.h>
#include <dpp/http_server_request.h>
#include <dpp/ssl_context.h>

namespace dpp {

/**
 * @brief Creates a simple HTTP server which listens on a TCP port for a
 * plaintext or SSL incoming request, and passes that request to a callback
 * to generate the response.
 */
struct DPP_EXPORT http_server : public socket_listener<http_server_request> {

	/**
	 * @brief Request handler callback to use for all incoming HTTP(S) requests
	 */
	http_server_request_event request_handler;

	/**
	 * @brief Port we are listening on
	 */
	uint16_t bound_port;

	/**
	 * @brief Constructor for creation of a HTTP(S) server
	 * @param creator Cluster creator
	 * @param address address to bind to, use "0.0.0.0" to bind to all local addresses
	 * @param port port to bind to. You should generally use a port > 1024.
	 * @param handle_request Callback to call for each pending request
	 * @param private_key Private key PEM file for HTTPS/SSL. If empty, a plaintext server is created
	 * @param public_key Public key PEM file for HTTPS/SSL. If empty, a plaintext server is created
	 */
	http_server(cluster* creator, const std::string_view address, uint16_t port, http_server_request_event handle_request, const std::string& private_key = "", const std::string& public_key = "");

	/**
	 * @brief Emplace a new request into the connection pool
	 * @param newfd file descriptor of new request
	 */
	void emplace(socket newfd) override;

	/**
	 * @brief Destructor
	 */
	virtual ~http_server() {
		detail::release_ssl_context(bound_port);
	}
};

}
