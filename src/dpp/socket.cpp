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

#include <dpp/socket.h>
#include <dpp/sslconnection.h>
#include <cstring>

namespace dpp {

address_t::address_t(const std::string_view ip, uint16_t port) {
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = inet_addr(ip.data());
	std::memcpy(&socket_addr, &address, sizeof(address));
}

sockaddr* address_t::get_socket_address() {
	return &socket_addr;
}

size_t address_t::size() {
	return sizeof(sockaddr_in);
}

uint16_t address_t::get_port(socket fd) {
	socklen_t len = size();
	if (getsockname(fd, &socket_addr, &len) > -1) {
		sockaddr_in sin{};
		std::memcpy(&sin, &socket_addr, sizeof(sockaddr_in));
		return ntohs(sin.sin_port);
	}
	return 0;
}

raii_socket::raii_socket(raii_socket_type type) : fd(::socket(AF_INET, type == rst_udp ? SOCK_DGRAM : SOCK_STREAM, 0)) {
}

raii_socket::raii_socket(socket plain_fd) {
	fd = plain_fd;
}

raii_socket::~raii_socket() {
	close_socket(fd);
}

bool raii_socket::bind(address_t address) {
	return ::bind(fd, address.get_socket_address(), address.size()) >= 0;

}

template <typename T> bool raii_socket::set_option(int level, int name, T value) {
	return ::setsockopt(fd, level, name, reinterpret_cast<char*>(&value), sizeof(value)) == 0;
}

template bool raii_socket::set_option<int>(int, int, int);
template bool raii_socket::set_option<bool>(int, int, bool);
template bool raii_socket::set_option<socklen_t>(int, int, socklen_t);

bool raii_socket::listen() {
	return ::listen(fd, SOMAXCONN) >= 0;
}

socket raii_socket::accept() {
	sockaddr_in addr;
	socklen_t addr_len{sizeof(sockaddr_in)};
	return ::accept(fd, (struct sockaddr*)&addr, &addr_len);
}

}