/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
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
#else
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#endif
#include <sys/types.h>
#include <string>
#include <unordered_map>
#include <cstring>
#include <memory>
#include <dpp/socket.h>

namespace dpp {

	/**
	 * @brief Represents a cached DNS result.
	 * Used by the ssl_connection class to store cached copies of dns lookups.
	 */
	struct DPP_EXPORT dns_cache_entry {
		/**
		 * @brief Resolved address metadata
		 */
		addrinfo addr;

		/**
		 * @brief Resolved address as string.
		 * The metadata is needed to know what type of address it is.
		 * Do not do silly stuff like just looking to see if '.' is in it!
		 */
		std::string resolved_addr;

		/**
		 * @brief Time at which this cache entry is invalidated
		 */
		time_t expire_timestamp;

		/**
		 * @brief Get address length
		 * @return address length
		 */
		[[nodiscard]] int size() const;

		/**
		 * @brief Get the address_t that corresponds to this cache entry
		 * for use when connecting with ::connect()
		 * @param port Port number to connect to
		 * @return address_t prefilled with the IP and port number
		 */
		[[nodiscard]] const address_t get_connecting_address(uint16_t port) const;

		/**
		 * @brief Allocate a socket file descriptor for the given dns address
		 * @return File descriptor ready for calling connect(), or INVALID_SOCKET
		 * on failure.
		 */
		[[nodiscard]] socket make_connecting_socket() const;
	};

	/**
	 * @brief Cache container type
	 */
	using dns_cache_t = std::unordered_map<std::string, std::unique_ptr<dns_cache_entry>>;

	/**
	 * @brief Resolve a hostname to an addrinfo
	 * 
	 * @param hostname Hostname to resolve
	 * @param port A port number or named service, e.g. "80"
	 * @return dns_cache_entry* First IP address associated with the hostname DNS record
	 * @throw dpp::connection_exception On failure to resolve hostname
	 */
	DPP_EXPORT const dns_cache_entry *resolve_hostname(const std::string &hostname, const std::string &port);
	}
