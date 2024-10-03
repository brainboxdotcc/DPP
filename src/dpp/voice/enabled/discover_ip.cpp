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

namespace dpp {

/**
 * https://discord.com/developers/docs/topics/voice-connections#ip-discovery
 */

#pragma pack(push, 1)
struct ip_discovery_packet {
	uint16_t type;
	uint16_t length;
	uint32_t ssrc;
	char address[64]{0}; // NOLINT
	uint16_t port;

	ip_discovery_packet() = delete;

	ip_discovery_packet(uint32_t _ssrc) :
		type(htons(0x01)), length(htons(sizeof(ip_discovery_packet) - sizeof(type) - sizeof(length))),
		ssrc(htonl(_ssrc)), port(0) {
		std::memset(&address, 0, sizeof(address));
	}
};
#pragma pack(pop)

std::string discord_voice_client::discover_ip() {

	if (!external_ip.empty()) {
		return external_ip;
	}

	dpp::socket newfd = SOCKET_ERROR;
	ip_discovery_packet discovery((uint32_t)this->ssrc);

	if ((newfd = ::socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		sockaddr_in servaddr{};
		memset(&servaddr, 0, sizeof(sockaddr_in));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(0);
		if (bind(newfd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
			log(ll_warning, "Could not bind socket for IP discovery");
			return "";
		}
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(this->port);
		servaddr.sin_addr.s_addr = inet_addr(this->ip.c_str());
		if (::connect(newfd, (const sockaddr*)&servaddr, sizeof(sockaddr_in)) < 0) {
			log(ll_warning, "Could not connect socket for IP discovery");
			return "";
		}
		if (::send(newfd, reinterpret_cast<const char*>(&discovery), sizeof(discovery), 0) == -1) {
			log(ll_warning, "Could not send packet for IP discovery");
			return "";
		}
		if (recv(newfd, reinterpret_cast<char*>(&discovery), sizeof(discovery), 0) == -1) {
			log(ll_warning, "Could not receive packet for IP discovery");
			return "";
		}

		close_socket(newfd);

		return {discovery.address, strlen(discovery.address)};
	}
	return {};
}

}