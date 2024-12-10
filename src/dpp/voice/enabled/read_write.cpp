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

void discord_voice_client::send(const char* packet, size_t len, uint64_t duration, bool send_now) {
	if (!send_now) [[likely]] {
		voice_out_packet frame;
		frame.packet.assign(packet, packet + len);
		frame.duration = duration;

		std::lock_guard<std::mutex> lock(this->stream_mutex);
		outbuf.emplace_back(frame);
	} else [[unlikely]] {
		this->udp_send(packet, len);
	}
}

int discord_voice_client::udp_send(const char* data, size_t length) {
	return static_cast<int>(sendto(
		this->fd,
		data,
		static_cast<int>(length),
		0,
		destination.get_socket_address(),
		destination.size()
	));
}

int discord_voice_client::udp_recv(char* data, size_t max_length)
{
	return static_cast<int>(recv(this->fd, data, static_cast<int>(max_length), 0));
}

}
