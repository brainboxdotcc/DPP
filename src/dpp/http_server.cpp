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
#include <dpp/http_server.h>
#include <memory>

namespace dpp {

http_server::http_server(cluster* owner, const std::string_view address, uint16_t port, http_server_request_event handle_request, const std::string& private_key, const std::string& public_key)
 : socket_listener<http_server_request>(owner, address, port, private_key.empty() ? li_plaintext : li_ssl, private_key, public_key), request_handler(handle_request), bound_port(port)
{
}

void http_server::emplace(socket newfd) {
	connections.emplace(newfd, std::make_unique<http_server_request>(creator, newfd, bound_port, plaintext, private_key_file, public_key_file, request_handler));
}

}
