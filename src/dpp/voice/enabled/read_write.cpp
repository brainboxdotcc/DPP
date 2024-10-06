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
#include <dpp/isa_detection.h>
#include <dpp/discordvoiceclient.h>
#include "../../dave/encryptor.h"
#include "enabled.h"

namespace dpp {

dpp::socket discord_voice_client::want_write() {
	std::lock_guard<std::mutex> lock(this->stream_mutex);
	if (!this->paused && !outbuf.empty()) {
		return fd;
	} else {
		return INVALID_SOCKET;
	}
}

dpp::socket discord_voice_client::want_read() {
	return fd;
}


void discord_voice_client::send(const char* packet, size_t len, uint64_t duration) {
	std::lock_guard<std::mutex> lock(this->stream_mutex);
	voice_out_packet frame;
	frame.packet = std::string(packet, len);
	frame.duration = duration;
	outbuf.emplace_back(frame);
}

int discord_voice_client::udp_send(const char* data, size_t length) {
	sockaddr_in servaddr{};
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(this->port);
	servaddr.sin_addr.s_addr = inet_addr(this->ip.c_str());
	return (int) sendto(this->fd, data, (int)length, 0, (const sockaddr*)&servaddr, (int)sizeof(sockaddr_in));
}

int discord_voice_client::udp_recv(char* data, size_t max_length)
{
	return (int) recv(this->fd, data, (int)max_length, 0);
}

}