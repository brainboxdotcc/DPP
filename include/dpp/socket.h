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
#ifdef _WIN32
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <io.h>
	#define poll(fds, nfds, timeout) WSAPoll(fds, nfds, timeout)
	#define pollfd WSAPOLLFD
#else
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
#endif
#include <string_view>
#include <cstdint>


namespace dpp
{
/**
 * @brief Represents a socket file descriptor.
 * This is used to ensure parity between windows and unix-like systems.
 */
#ifndef _WIN32
	using socket = int;
#else
	using socket = SOCKET;
#endif

#ifndef SOCKET_ERROR
/**
 * @brief Represents a socket in error state
 */
#define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
/**
 * @brief Represents a socket which is not yet assigned
 */
#define INVALID_SOCKET ~0
#endif

/**
 * @brief Represents an IPv4 address for use with socket functions such as
 * bind().
 *
 * Avoids type punning with C style casts from sockaddr_in to sockaddr pointers.
 */
class DPP_EXPORT address_t {
	/**
	 * @brief Internal sockaddr struct
	 */
	sockaddr socket_addr{};

public:

	/**
	 * @brief Create a new address_t
	 * @param ip IPv4 address
	 * @param port Port number
	 * @note Leave both as defaults to create a default bind-to-any setting
	 */
	address_t(const std::string_view ip = "0.0.0.0", uint16_t port = 0);

	/**
	 * @brief Get sockaddr
	 * @return sockaddr pointer
	 */
	[[nodiscard]] sockaddr *get_socket_address();

	/**
	 * @brief Returns size of sockaddr_in
	 * @return sockaddr_in size
	 * @note It is important the size this returns is sizeof(sockaddr_in) not
	 * sizeof(sockaddr), this is NOT a bug but requirement of C socket functions.
	 */
	[[nodiscard]] size_t size();

	/**
	 * @brief Get the port bound to a file descriptor
	 * @param fd File descriptor
	 * @return Port number, or 0 if no port bound
	 */
	[[nodiscard]] uint16_t get_port(socket fd);
};

enum raii_socket_type {
	rst_udp,
	rst_tcp,

};

/**
 * @brief Allocates a dpp::socket, closing it on destruction
 */
struct DPP_EXPORT raii_socket {
	/**
	 * @brief File descriptor
	 */
	socket fd;

	/**
	 * @brief Construct a socket.
	 * Calls socket() and returns a new file descriptor
	 */
	raii_socket(raii_socket_type type = rst_udp);

	/**
	 * @brief Convert an established fd to an raii_socket
	 * @param plain_fd
	 */
	raii_socket(socket plain_fd);

	/**
	 * @brief Non-copyable
	 */
	raii_socket(raii_socket&) = delete;

	/**
	 * @brief Non-movable
	 */
	raii_socket(raii_socket&&) = delete;

	/**
	 * @brief Sets the value of a socket option.
	 * @tparam T type to set option for
	 * @param level The level at which to change the socket options
	 * @param name The option to change the value of
	 * @param value The value to set
	 * @return True if set successfully
	 */
	template <typename T> bool set_option(int level, int name, T value);

	/**
	 * @brief Bind socket to IP/port
	 * @param address address to bind to
	 * @return true on success
	 */
	bool bind(address_t address);

	/**
	 * @brief Listen on previously bound port
	 * @return true on success
	 */
	bool listen();

	/**
	 * @brief Accept a pending connection on listening socket
	 * @return new connection file descriptor
	 */
	socket accept();

	/**
	 * @brief Non-copyable
	 */
	raii_socket operator=(raii_socket&) = delete;

	/**
	 * @brief Non-movable
	 */
	raii_socket operator=(raii_socket&&) = delete;

	/**
	 * @brief Destructor
	 * Frees the socket by closing it
	 */
	~raii_socket();
};


}
