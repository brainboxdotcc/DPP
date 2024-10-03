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
	: websocket_client(_host.substr(0, _host.find(':')), _host.substr(_host.find(':') + 1, _host.length()), "/?v=" + std::to_string(voice_protocol_version), OP_TEXT),
	runner(nullptr),
	connect_time(0),
	mixer(nullptr),
	port(0),
	ssrc(0),
	timescale(1000000),
	paused(false),
	encoder(nullptr),
	repacketizer(nullptr),
	fd(INVALID_SOCKET),
	sequence(0),
	receive_sequence(-1),
	timestamp(0),
	packet_nonce(1),
	last_timestamp(std::chrono::high_resolution_clock::now()),
	sending(false),
	tracks(0),
	creator(_cluster),
	terminating(false),
	heartbeat_interval(0),
	last_heartbeat(time(nullptr)),
	token(_token),
	sessionid(_session_id),
	server_id(_server_id),
	channel_id(_channel_id)
{
	throw dpp::voice_exception(err_no_voice_support, "Voice support not enabled in this build of D++");
}

}