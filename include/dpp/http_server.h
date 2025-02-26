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
#include <dpp/socket_listener.h>
#include <dpp/http_server_request.h>

namespace dpp {

/**
 * @brief Listens on a TCP socket for new connections, and whenever a new connection is
 * received, accept it and spawn a new connection of type T.
 * @tparam T type for socket connection, must be derived from ssl_connection
 */
struct http_server : public socket_listener<http_server_request> {
	http_server_request_event request_handler;
	
	http_server(cluster* creator, const std::string_view address, uint16_t port, http_server_request_event handle_request, socket_listener_type type = li_plaintext, const std::string& private_key = "", const std::string& public_key = "");

	void emplace(socket newfd) override;
};

}
