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

#include <dpp/exception.h>
#include <dpp/discordvoiceclient.h>
#include "enabled.h"

#ifdef _WIN32
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <io.h>
	#define poll(fds, nfds, timeout) WSAPoll(fds, nfds, timeout)
	#define pollfd WSAPOLLFD
#else
	#include <poll.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
#endif

namespace dpp {

/**
 * @brief Represents an IP discovery packet sent to Discord or received
 * from Discord.
 *
 * https://discord.com/developers/docs/topics/voice-connections#ip-discovery
 */
struct ip_discovery_packet {

	/**
	 * @brief Maximum size of packet
	 */
	static constexpr int DISCOVERY_PACKET_SIZE = 74;

	/**
	 * @brief Maximum length of IP address string
	 */
	static constexpr int ADDRESS_BUFFER_SIZE = 64;

	/**
	 * @brief Type of packet
	 */
	uint16_t type;

	/**
	 * @brief Length of packet
	 */
	uint16_t length;

	/**
	 * @brief SSRC of sender
	 */
	uint32_t ssrc;

	/**
	 * @brief Address buffer, contains IP address in returned packet
	 */
	char address[ADDRESS_BUFFER_SIZE]{0}; // NOLINT

	/**
	 * @brief Port number, contains port in returned packet
	 */
	uint16_t port;

	/**
	 * @brief Construct discovery packet from inbound recv() buffer contents
	 * @param packet_buffer recv buffer contents of at least ADDRESS_BUFFER_SIZE bytes
	 */
	ip_discovery_packet(char* packet_buffer)
		: type(ntohs(packet_buffer[0] << 8 | packet_buffer[1])),
		length(ntohs(packet_buffer[2] << 8 | packet_buffer[3])),
		ssrc(ntohl(packet_buffer[4] << 24 | packet_buffer[5] << 16 | packet_buffer[6] << 8 | packet_buffer[7])),
		port(ntohs(packet_buffer[72] << 8 | packet_buffer[73]))
	{
		std::memcpy(address, packet_buffer + 8, ADDRESS_BUFFER_SIZE);
	}

	/**
	 * @brief Build a const char* buffer for sending with send() to make a request
	 * @return char buffer
	 */
	const std::array<char, DISCOVERY_PACKET_SIZE> build_buffer() {
		std::array<char, DISCOVERY_PACKET_SIZE> buffer{0};
		buffer[0] = ((type & 0xff00) >> 8) & 0xff;
		buffer[1] = type & 0xff;
		buffer[2] = (length & 0xff00) >> 8;
		buffer[3] = length & 0xff;
		buffer[4] = ((ssrc & 0xff000000) >> 24) & 0xff;
		buffer[5] = ((ssrc & 0x00ff0000) >> 16) & 0xff;
		buffer[6] = ((ssrc & 0x0000ff00) >> 8) & 0xff;
		buffer[7] = ssrc & 0x000000ff;
		return buffer;
	}

	/**
	 * @brief Deleted default constructor
	 */
	ip_discovery_packet() = delete;

	/**
	 * @brief Build a request packet for a given SSRC.
	 * type and length will be initialised correctly and the address
	 * buffer will be zeroed.
	 * @param _ssrc SSRC value
	 */
	ip_discovery_packet(uint32_t _ssrc) :
		/* Packet length is size of header minus type and length fields, usually 70 bytes */
		type(0x01), length(DISCOVERY_PACKET_SIZE - sizeof(type) - sizeof(length)),
		ssrc(_ssrc), port(0) {
		std::memset(&address, 0, ADDRESS_BUFFER_SIZE);
	}
};

constexpr int discovery_timeout = 1000;

std::string discord_voice_client::discover_ip() {

	if (!external_ip.empty()) {
		return external_ip;
	}

	raii_socket socket;
	ip_discovery_packet discovery(this->ssrc);

	if (socket.fd >= 0) {
		address_t bind_any;
		if (bind(socket.fd, bind_any.get_socket_address(), bind_any.size()) < 0) {
			log(ll_warning, "Could not bind socket for IP discovery");
			return "";
		}
		address_t bind_port(this->ip, this->port);
#ifndef _WIN32
		if (::connect(socket.fd, bind_port.get_socket_address(), bind_port.size()) < 0) {
#else
		if (WSAConnect(socket.fd, bind_port.get_socket_address(), bind_port.size(), nullptr, nullptr, nullptr, nullptr) < 0) {
#endif
			log(ll_warning, "Could not connect socket for IP discovery");
			return "";
		}
		if (::send(socket.fd, discovery.build_buffer().data(), ip_discovery_packet::DISCOVERY_PACKET_SIZE, 0) == -1) {
			log(ll_warning, "Could not send packet for IP discovery");
			return "";
		}
		/* Wait one second for receipt of IP detection packet response */
		pollfd pfd{};
		pfd.fd = socket.fd;
		pfd.events = POLLIN;
		int ret = ::poll(&pfd, 1, discovery_timeout);
		switch (ret) {
			case -1:
				log(ll_warning, "poll() error on IP discovery");
				return "";
			case 0:
				log(ll_warning, "Timed out in IP discovery");
				return "";
			default:
				char buffer[ip_discovery_packet::DISCOVERY_PACKET_SIZE]{0};
				if (recv(socket.fd, buffer, sizeof(buffer), 0) == -1) {
					log(ll_warning, "Could not receive packet for IP discovery");
					return "";
				}
				ip_discovery_packet inbound_packet(buffer);
				return {inbound_packet.address};
		}
	}
	return {};
}

}
