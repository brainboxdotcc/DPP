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

#include "stub.h"

namespace dpp {

	discord_voice_client::discord_voice_client(dpp::cluster* _cluster, snowflake _channel_id, snowflake _server_id, const std::string &_token, const std::string &_session_id, const std::string &_host, bool enable_dave)
		: websocket_client(_cluster, _host.substr(0, _host.find(':')), _host.substr(_host.find(':') + 1, _host.length()), "/?v=" + std::to_string(voice_protocol_version), OP_TEXT)
	{
		throw dpp::voice_exception(err_no_voice_support, "Voice support not enabled in this build of D++");
	}

	void discord_voice_client::voice_courier_loop(discord_voice_client& client, courier_shared_state_t& shared_state) {
	}

	void discord_voice_client::cleanup() {
	}

	void discord_voice_client::run() {
	}

	bool discord_voice_client::voice_payload::operator<(const voice_payload& other) const {
		return false;
	}

	bool discord_voice_client::handle_frame(const std::string &data, ws_opcode opcode) {
		return false;
	}

	void discord_voice_client::read_ready() {
	}

	void discord_voice_client::write_ready() {
	}

	discord_voice_client& discord_voice_client::send_audio_raw(uint16_t* audio_data, const size_t length)  {
		return *this;
	}

	discord_voice_client& discord_voice_client::send_audio_opus(const uint8_t* opus_packet, const size_t length, uint64_t duration, bool send_now) {
		return *this;
	}

	discord_voice_client& discord_voice_client::send_audio_opus(const uint8_t* opus_packet, const size_t length) {
		return *this;
	}

	void discord_voice_client::send(const char* packet, size_t len, uint64_t duration, bool send_now) {
	}

	int discord_voice_client::udp_send(const char* data, size_t length) {
		return -1;
	}

	int discord_voice_client::udp_recv(char* data, size_t max_length) {
		return -1;
	}

	size_t discord_voice_client::encode(uint8_t *input, size_t inDataSize, uint8_t *output, size_t &outDataSize) {
		return 0;
	}

	std::string discord_voice_client::discover_ip() {
		return "";
	}

	void discord_voice_client::setup() {
	}

	void discord_voice_client::on_disconnect() {
	}

}
