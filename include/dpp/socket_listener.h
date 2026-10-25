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
#include <dpp/sslconnection.h>
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
	/**
	 * @brief The listening socket for incoming connections
	 */
	raii_socket fd;

	/**
	 * @brief Active connections for the server of type T
	 */
	std::unordered_map<socket, std::unique_ptr<T>> connections;

	/**
	 * @brief Cluster creator
	 */
	cluster* creator{nullptr};

	/**
	 * @brief True if plain text connections to the server are allowed
	 */
	bool plaintext{true};

	/**
	 * @brief Private key PEM file path, if running an SSL server
	 */
	std::string private_key_file;

	/**
	 * @brief Public key PEM file path, if running an SSL server
	 */
	std::string public_key_file;
	
	/**
	 * @brief Event to handle socket removal from the connection map
	 */
	event_handle close_event;

	/**
	 * @brief Socket events for listen socket in the socket engine
	 */
	socket_events events;

	/**
	 * @brief Create a new socket listener (TCP server)
	 * @param owner Owning cluster
	 * @param address IP address to bind the listening socket to, use 0.0.0.0 to bind all interfaces
	 * @param port Port number to bind the listening socket to
	 * @param type Type of server, plaintext or SSL
	 * @param private_key For SSL servers, a path to the PEM private key file
	 * @param public_key For SSL servers, a path to the PEM public key file
	 * @throws connection_exception on failure to bind or listen to the port/interface
	 */
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
			[](socket, const struct socket_events&) { },
			[](socket, const struct socket_events&, int) { }
		);
		owner->socketengine->register_socket(events);

		close_event = creator->on_socket_close([this](const socket_close_t& event) {
			connections.erase(event.fd);
		});
	}

	/**
	 * @brief Destructor, detaches on_socket_close event
	 */
	~socket_listener() {
		creator->on_socket_close.detach(close_event);
	}

	/**
	 * @brief Handle a new incoming socket with accept()
	 * Accepts a new connection, and calls emplace() if valid
	 * @param sfd File descriptor for listening socket
	 * @param e socket events for the listening socket
	 */
	virtual void handle_accept(socket sfd, const struct socket_events &e) {
		socket new_fd{fd.accept()};
		if (new_fd >= 0) {
			emplace(new_fd);
		}
	}

	/**
	 * @brief Emplace a new connection into the connection map for the server.
	 * This is a factory function which must be implemented by the deriving class
	 * @param newfd File descriptor for new connection
	 */
	virtual void emplace(socket newfd) = 0;
};

}
