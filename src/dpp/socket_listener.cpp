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
#include <dpp/socket_listener.h>

namespace dpp {

template<typename T, typename U>
socket_listener<T, U>::socket_listener(cluster* owner, const std::string_view address, uint16_t port, socket_listener_type type, const std::string& private_key, const std::string& public_key)
 : creator(owner), plaintext(type == li_plaintext), private_key_file(private_key), public_key_file(public_key)
{
	if ((fd = ::socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		address_t bind_addr(address, port);
		if (bind(fd, bind_addr.get_socket_address(), bind_addr.size()) < 0) {
			// error
		}
		if (::listen(fd, SOMAXCONN) < 0) {
			// error
		}
		events = dpp::socket_events(
			fd,
			WANT_READ | WANT_ERROR,
			[this](socket fd, const struct socket_events &e) { 
				handle_accept(fd, e);
			},
			[this](socket, const struct socket_events) { },
			[this](socket, const struct socket_events, int) {
					// error ?
			}
		);
		owner->socketengine->register_socket(events);
		
		close_event = creator->on_socket_close([this](const socket_close_t& event) {
			connections.erase(event.fd);
		});
	}
}

template<typename T, typename U>
void socket_listener<T, U>::handle_accept(socket fd, const struct socket_events &e) {
	socket new_fd{INVALID_SOCKET};
	sockaddr_in addr;
	socklen_t addr_len{sizeof(sockaddr_in)};
	if ((new_fd = ::accept(fd, (struct sockaddr*)&addr, &addr_len)) < 0) {
		// error ?
	}
	emplace(new_fd);
}

template<typename T, typename U>
void socket_listener<T, U>::emplace(socket newfd) {
	connections.emplace(newfd, std::make_unique<T>(creator, newfd, plaintext, private_key_file, public_key_file));
}

template<typename T, typename U>
socket_listener<T, U>::~socket_listener() {
	creator->socketengine->delete_socket(fd);
	close_socket(fd);
	creator->on_socket_close.detach(close_event);
}

}
