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
#include <dpp/socket.h>
#include <dpp/sslclient.h>
#include <type_traits>
#include <memory>
#include <unordered_map>
#include <string_view>
#include <string>

namespace dpp {

enum socket_listener_type : uint8_t {
	li_plaintext,
	li_ssl,
};

/**
 * @brief Listens on a TCP socket for new connections, and whenever a new connection is
 * received, accept it and spawn a new connection of type T.
 * @tparam T type for socket connection, must be derived from ssl_connection
 */
template<typename T, typename = std::enable_if_t<std::is_base_of_v<ssl_connection, T>>>
struct socket_listener {
	raii_socket fd;
	std::unordered_map<socket, std::unique_ptr<T>> connections;
	cluster* creator{nullptr};
	bool plaintext{true};
	std::string private_key_file;
	std::string public_key_file;
	event_handle close_event;
	socket_events events;

	socket_listener(cluster* owner, const std::string_view address, uint16_t port, socket_listener_type type = li_plaintext, const std::string& private_key = "", const std::string& public_key = "")
	: fd(rst_tcp), creator(owner), plaintext(type == li_plaintext), private_key_file(private_key), public_key_file(public_key)
	{
		fd.set_option<int>(SOL_SOCKET, SO_REUSEADDR, 1);
		if (!fd.bind(address_t(address, port))) {
			// error
			throw dpp::connection_exception("Could not bind to " + std::string(address) + ":" + std::to_string(port));
		}
		if (!fd.listen()) {
			// error
			throw dpp::connection_exception("Could not listen for connections on " + std::string(address) + ":" + std::to_string(port));
		}
		events = dpp::socket_events(
			fd.fd,
			WANT_READ | WANT_ERROR,
			[this](socket sfd, const struct socket_events &e) {
				handle_accept(sfd, e);
			},
			[this](socket, const struct socket_events&) { },
			[this](socket, const struct socket_events&, int) { }
		);
		owner->socketengine->register_socket(events);

		close_event = creator->on_socket_close([this](const socket_close_t& event) {
			connections.erase(event.fd);
		});
	}


	~socket_listener() {
		creator->on_socket_close.detach(close_event);
	}

	virtual void handle_accept(socket sfd, const struct socket_events &e) {
		socket new_fd{fd.accept()};
		if (new_fd >= 0) {
			emplace(new_fd);
		}
	}

	virtual void emplace(socket newfd) = 0;
};

}
