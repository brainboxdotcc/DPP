/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
#include <dpp/zlibcontext.h> * Copyright 2021 Craig Edwards and D++ contributors
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
#include <string>
#include <fstream>
#include <dpp/exception.h>
#include <dpp/discordclient.h>
#include <dpp/cache.h>
#include <dpp/cluster.h>
#include <thread>
#include <dpp/json.h>
#include <dpp/etf.h>

#define PATH_UNCOMPRESSED_JSON "/?v=" DISCORD_API_VERSION "&encoding=json"
#define PATH_COMPRESSED_JSON "/?v=" DISCORD_API_VERSION "&encoding=json&compress=zlib-stream"
#define PATH_UNCOMPRESSED_ETF "/?v=" DISCORD_API_VERSION "&encoding=etf"
#define PATH_COMPRESSED_ETF "/?v=" DISCORD_API_VERSION "&encoding=etf&compress=zlib-stream"
#define STRINGIFY(a) STRINGIFY_(a)
#define STRINGIFY_(a) #a

#ifndef DPP_OS
	#define DPP_OS unknown
#endif

namespace dpp {

/**
 * @brief Used in IDENTIFY to indicate what a large guild is
 */
constexpr int LARGE_THRESHOLD = 250;

/**
 * @brief Resume constructor for websocket client
 */
discord_client::discord_client(discord_client &old, uint64_t sequence, const std::string& session_id)
	: websocket_client(old.owner, old.resume_gateway_url, "443", old.compressed ? (old.protocol == ws_json ? PATH_COMPRESSED_JSON : PATH_COMPRESSED_ETF) : (old.protocol == ws_json ? PATH_UNCOMPRESSED_JSON : PATH_UNCOMPRESSED_ETF)),
	  compressed(old.compressed),
	  zlib(nullptr),
	  connect_time(0),
	  ping_start(0.0),
	  etf(nullptr),
	  creator(old.owner),
	  heartbeat_interval(0),
	  last_heartbeat(time(nullptr)),
	  shard_id(old.shard_id),
	  max_shards(old.max_shards),
	  last_seq(sequence),
	  token(old.token),
	  intents(old.intents),
	  sessionid(session_id),
	  resumes(old.resumes),
	  reconnects(old.reconnects),
	  websocket_ping(old.websocket_ping),
	  ready(false),
	  last_heartbeat_ack(time(nullptr)),
	  protocol(old.protocol),
	  resume_gateway_url(old.resume_gateway_url)
{
	start_connecting();
}

discord_client::discord_client(dpp::cluster* _cluster, uint32_t _shard_id, uint32_t _max_shards, const std::string &_token, uint32_t _intents, bool comp, websocket_protocol_t ws_proto)
       : websocket_client(_cluster, _cluster->default_gateway, "443", comp ? (ws_proto == ws_json ? PATH_COMPRESSED_JSON : PATH_COMPRESSED_ETF) : (ws_proto == ws_json ? PATH_UNCOMPRESSED_JSON : PATH_UNCOMPRESSED_ETF)),
	compressed(comp),
	zlib(nullptr),
	connect_time(0),
	ping_start(0.0),
	etf(nullptr),
	creator(_cluster),
	heartbeat_interval(0),
	last_heartbeat(time(nullptr)),
	shard_id(_shard_id),
	max_shards(_max_shards),
	last_seq(0),
	token(_token),
	intents(_intents),
	resumes(0),
	reconnects(0),
	websocket_ping(0.0),
	ready(false),
	last_heartbeat_ack(time(nullptr)),
	protocol(ws_proto),
	resume_gateway_url(_cluster->default_gateway)
{
	start_connecting();
}

void discord_client::start_connecting() {
	etf = std::make_unique<etf_parser>();
	if (compressed) {
		zlib = std::make_unique<zlibcontext>();
	}
	websocket_client::connect();
}

void discord_client::cleanup()
{
}

void discord_client::on_disconnect()
{
	log(ll_trace, "discord_client::on_disconnect()");
	set_resume_hostname();
	if (sfd != INVALID_SOCKET) {
		log(dpp::ll_debug, "Lost connection to websocket on shard " + std::to_string(shard_id) + ", reconnecting...");
	}
	ssl_connection::close();
	owner->add_reconnect(this->shard_id);
}

uint64_t discord_client::get_decompressed_bytes_in()
{
	return zlib ? zlib->decompressed_total : 0;
}

void discord_client::set_resume_hostname()
{
	hostname = resume_gateway_url;
}

void discord_client::run()
{
	ready = false;
	message_queue.clear();
	ssl_connection::read_loop();
}

bool discord_client::handle_frame(const std::string &buffer, ws_opcode opcode)
{
	auto& data = const_cast<std::string&>(buffer);

	/* gzip compression is a special case */
	if (compressed) {
		/* Check that we have a complete compressed frame */
		if ((uint8_t)buffer[buffer.size() - 4] == 0x00 && (uint8_t)buffer[buffer.size() - 3] == 0x00 && (uint8_t)buffer[buffer.size() - 2] == 0xFF
		&& (uint8_t)buffer[buffer.size() - 1] == 0xFF) {
			auto result = zlib->decompress(buffer, decompressed);
			if (result != err_no_code_specified) {
				this->error(result);
				this->close();
				return false;
			}
			data = decompressed;
		} else {
			/* No complete compressed frame yet */
			return false;
		}
	}


	json j;
	
	/**
	 * This section parses the input frames from the websocket after they're decompressed.
	 * Note that both ETF and JSON parsers return an nlohmann::json object, so that the rest
	 * of the library or any user code does not need to be concerned with protocol differences.
	 * Generally, ETF is faster and consumes much less memory, but provides less opportunities
	 * to diagnose if it goes wrong.
	 */
	switch (protocol) {
		case ws_json:
			try {
				j = json::parse(data);
			}
			catch (const std::exception &e) {
				log(dpp::ll_error, "discord_client::handle_frame(JSON): " + std::string(e.what()) + " [" + data + "]");
				return true;
			}
		break;
		case ws_etf:
			try {
				j = etf->parse(data);
			}
			catch (const std::exception &e) {
				log(dpp::ll_error, "discord_client::handle_frame(ETF): " + std::string(e.what()) + " len=" + std::to_string(data.size()) + "\n" + dpp::utility::debug_dump((uint8_t*)data.data(), data.size()));
				return true;
			}
		break;
	}

	//log(dpp::ll_trace, "R: " + j.dump());

	auto seq = j.find("s");
	if (seq != j.end() && !seq->is_null()) {
		last_seq = seq->get<uint64_t>();
	}

	auto o = j.find("op");
	if (o != j.end() && !o->is_null()) {
		shard_frame_type op = o->get<shard_frame_type>();

		switch (op) {
			case ft_invalid_session:
				/* Reset session state and fall through to ft_hello */
				op = ft_hello;
				log(dpp::ll_debug, "Failed to resume session " + sessionid + ", will reidentify");
				this->sessionid.clear();
				this->last_seq = 0;
				/* No break here, falls through to state ft_hello to cause a re-identify */
				[[fallthrough]];
			case ft_hello: {
				/* Need to check carefully for the existence of this before we try to access it! */
				auto d = j.find("d");
				if (d != j.end()) {
					auto heartbeat = d->find("heartbeat_interval");
					if (heartbeat != d->end() && !heartbeat->is_null())
						this->heartbeat_interval = heartbeat->get<uint32_t>();
				}

				if (last_seq != 0U && !sessionid.empty()) {
					/* Resume */
					log(dpp::ll_debug, "Resuming session " + sessionid + " with seq=" + std::to_string(last_seq));
					json obj = {
						{"op", ft_resume},
						{"d",  {
							       {"token", this->token},
							       {"session_id", this->sessionid},
							       {"seq", this->last_seq}
						       }
						}
					};
					this->write(jsonobj_to_string(obj), protocol == ws_etf ? OP_BINARY : OP_TEXT);
					resumes++;
				} else {
					/* Full connect */
					auto connect_now = [this]() {
						log(dpp::ll_debug, "Connecting new session...");
						json obj = {
							{"op", ft_identify},
							{"d",
							       {
								       {"token", this->token},
								       {"properties",
									       {
										       {"os", STRINGIFY(DPP_OS)},
										       {"browser", "D++"},
										       {"device", "D++"}
									       }
								       },
								       {"shard", json::array({shard_id, max_shards})},
								       {"compress", false},
								       {"large_threshold", LARGE_THRESHOLD},
								       {"intents", this->intents}
							       }
							}
						};
						this->write(jsonobj_to_string(obj), protocol == ws_etf ? OP_BINARY : OP_TEXT);
						this->connect_time = creator->last_identify = time(nullptr);
						reconnects++;
					};
					if (time(nullptr) < creator->last_identify + RECONNECT_INTERVAL) {
						owner->start_timer([this, connect_now](timer h) {
							owner->stop_timer(h);
							connect_now();
						}, (creator->last_identify + RECONNECT_INTERVAL) - time(nullptr));
					} else {
						connect_now();
					}
				}
				this->last_heartbeat_ack = time(nullptr);
				websocket_ping = 0;
			}
			break;
			case ft_dispatch: {
				std::string event = j["t"];
				handle_event(event, j, data);
			}
			break;
			case ft_reconnect:
				message_queue.clear();
				throw dpp::connection_exception("Reconnection requested, closing session " + sessionid);
			/* Heartbeat ack */
			case ft_heartbeat_ack:
				this->last_heartbeat_ack = time(nullptr);
				websocket_ping = utility::time_f() - ping_start;
			break;
			case ft_heartbeat:
			case ft_identify:
			case ft_presence:
			case ft_voice_state_update:
			case ft_resume:
			case ft_request_guild_members:
			case ft_request_soundboard_sounds:
				throw dpp::connection_exception("Received invalid opcode on websocket for session " + sessionid);
		}
	}
	return true;
}

dpp::utility::uptime discord_client::get_uptime()
{
	return {time(nullptr) - connect_time};
}

bool discord_client::is_connected()
{
	return (this->get_state() == CONNECTED) && (this->ready);
}

void discord_client::error(uint32_t errorcode)
{
	const static std::map<uint32_t, std::string> errortext = {
		{ 1000, "Socket shutdown" },
		{ 1001, "Client is leaving" },
		{ 1002, "Endpoint received a malformed frame" },
		{ 1003, "Endpoint received an unsupported frame" },
		{ 1004, "Reserved code" },
		{ 1005, "Expected close status, received none" },
		{ 1006, "No close code frame has been received" },
		{ 1007, "Endpoint received inconsistent message (e.g. malformed UTF-8)" },
		{ 1008, "Generic error" },
		{ 1009, "Endpoint won't process large frame" },
		{ 1010, "Client wanted an extension which server did not negotiate" },
		{ 1011, "Internal server error while operating" },
		{ 1012, "Server/service is restarting" },
		{ 1013, "Temporary server condition forced blocking client's request" },
		{ 1014, "Server acting as gateway received an invalid response" },
		{ 1015, "Transport Layer Security handshake failure" },
		{ 4000, "Unknown error" },
		{ 4001, "Unknown opcode" },
		{ 4002, "Decode error" },
		{ 4003, "Not authenticated" },
		{ 4004, "Authentication failed" },
		{ 4005, "Already authenticated" },
		{ 4007, "Invalid seq" },
		{ 4008, "Rate limited" },
		{ 4009, "Session timed out" },
		{ 4010, "Invalid shard" },
		{ 4011, "Sharding required" },
		{ 4012, "Invalid API version" },
		{ 4013, "Invalid intent(s)" },
		{ 4014, "Disallowed intent(s)" },
		{ 6000, "ZLib Stream Error" },
		{ 6001, "ZLib Data Error" },
		{ 6002, "ZLib Memory Error" },
		{ 6666, "Hell freezing over" }
	};
	std::string error = "Unknown error";
	auto i = errortext.find(errorcode);
	if (i != errortext.end()) {
		error = i->second;
	}
	log(dpp::ll_warning, "OOF! Error from underlying websocket: " + std::to_string(errorcode) + ": " + error);
	this->close();
}

void discord_client::log(dpp::loglevel severity, const std::string &msg) const
{
	if (!creator->on_log.empty()) {
		/* Pass to user if they've hooked the event */
		dpp::log_t logmsg(creator, shard_id, msg);
		logmsg.severity = severity;
		logmsg.message = msg;
		size_t pos{0};
		while ((pos = logmsg.message.find(token, pos)) != std::string::npos) {
			logmsg.message.replace(pos, token.length(), "*****");
			pos += 5;
		}
		creator->on_log.call(logmsg);
	}
}

void discord_client::queue_message(const std::string &j, bool to_front)
{
	std::unique_lock locker(queue_mutex);
	if (to_front) {
		message_queue.emplace_front(j);
	} else {
		message_queue.emplace_back(j);
	}
}

discord_client& discord_client::clear_queue()
{
	std::unique_lock locker(queue_mutex);
	message_queue.clear();
	return *this;
}

size_t discord_client::get_queue_size()
{
	std::shared_lock locker(queue_mutex);
	return message_queue.size();
}

void discord_client::one_second_timer()
{
	websocket_client::one_second_timer();

	/* This all only triggers if we are connected (have completed websocket, and received READY or RESUMED) */
	if (this->is_connected()) {

		/* If we stopped getting heartbeat acknowledgements, this means the connections is dead.
		 * This can happen to TCP connections which is why we have heartbeats in the first place.
		 * Miss two ACKS, forces a reconnection.
		 */
		if ((time(nullptr) - this->last_heartbeat_ack) > heartbeat_interval * 2) {
			log(dpp::ll_warning, "Missed heartbeat ACK, forcing reconnection to session " + sessionid);
			message_queue.clear();
			close_socket(sfd);
			return;
		}

		/* Rate limit outbound messages, 1 every odd second, 2 every even second */
		for (int x = 0; x < (time(nullptr) % 2) + 1; ++x) {
			std::unique_lock locker(queue_mutex);
			if (message_queue.size()) {
				std::string message = message_queue.front();
				message_queue.pop_front();
				/* Checking here by string comparison saves us having to deserialise the json
				 * to find pings in our queue.
				 */
				if (!last_ping_message.empty() && message == last_ping_message) {
					ping_start = utility::time_f();
					last_ping_message.clear();
				}
				this->write(message, protocol == ws_etf ? OP_BINARY : OP_TEXT);
			}
		}

		/* Send pings (heartbeat opcodes) before each interval. We send them slightly more regular than expected,
		 * just to be safe.
		 */
		if (this->heartbeat_interval && this->last_seq) {
			/* Check if we're due to emit a heartbeat */
			if (time(nullptr) > last_heartbeat + ((heartbeat_interval / 1000.0) * 0.75)) {
				last_ping_message = jsonobj_to_string(json({{"op", ft_heartbeat}, {"d", last_seq}}));
				queue_message(last_ping_message, true);
				last_heartbeat = time(nullptr);
			}
		}
	}
}

uint64_t discord_client::get_guild_count() {
	uint64_t total = 0;
	dpp::cache<guild>* c = dpp::get_guild_cache();
	/* IMPORTANT: We must lock the container to iterate it */
	std::shared_lock l(c->get_mutex());
	std::unordered_map<snowflake, guild*>& gc = c->get_container();
	for (auto g = gc.begin(); g != gc.end(); ++g) {
		dpp::guild* gp = (dpp::guild*)g->second;
		if (gp->shard_id == this->shard_id) {
			total++;
		}
	}
	return total;
}

uint64_t discord_client::get_member_count() {
	uint64_t total = 0;
	dpp::cache<guild>* c = dpp::get_guild_cache();
	/* IMPORTANT: We must lock the container to iterate it */
	std::shared_lock l(c->get_mutex());
	std::unordered_map<snowflake, guild*>& gc = c->get_container();
	for (auto g = gc.begin(); g != gc.end(); ++g) {
		dpp::guild* gp = (dpp::guild*)g->second;
		if (gp->shard_id == this->shard_id) {
			if (creator->cache_policy.user_policy == dpp::cp_aggressive) {
				/* We can use actual member count if we are using full user caching */
				total += gp->members.size();
			} else {
				/* Otherwise we use approximate guild member counts from guild_create */
				total += gp->member_count;
			}
		}
	}
	return total;
}

uint64_t discord_client::get_channel_count() {
	uint64_t total = 0;
	dpp::cache<guild>* c = dpp::get_guild_cache();
	/* IMPORTANT: We must lock the container to iterate it */
	std::shared_lock l(c->get_mutex());
	std::unordered_map<snowflake, guild*>& gc = c->get_container();
	for (auto g = gc.begin(); g != gc.end(); ++g) {
		dpp::guild* gp = (dpp::guild*)g->second;
		if (gp->shard_id == this->shard_id) {
			total += gp->channels.size();
		}
	}
	return total;
}

discord_client& discord_client::connect_voice(snowflake guild_id, snowflake channel_id, bool self_mute, bool self_deaf, bool enable_dave) {
#ifdef HAVE_VOICE
	std::unique_lock lock(voice_mutex);
	if (connecting_voice_channels.find(guild_id) != connecting_voice_channels.end()) {
		if (connecting_voice_channels[guild_id]->channel_id == channel_id) {
			log(ll_debug, "Requested the bot connect to voice channel " + std::to_string(channel_id) + " on guild " + std::to_string(guild_id) + ", but it seems we are already on this VC");
			return *this;
		}
	}
	connecting_voice_channels[guild_id] = std::make_unique<voiceconn>(this, channel_id, enable_dave);
	/* Once sent, this expects two events (in any order) on the websocket:
	* VOICE_SERVER_UPDATE and VOICE_STATUS_UPDATE
	*/
	log(ll_debug, "Sending op 4 to join VC, guild " + std::to_string(guild_id) + " channel " + std::to_string(channel_id) + (enable_dave ? " WITH DAVE" : ""));
	queue_message(jsonobj_to_string(json({
		{ "op", ft_voice_state_update },
		{ "d", {
				{ "guild_id", std::to_string(guild_id) },
				{ "channel_id", std::to_string(channel_id) },
				{ "self_mute", self_mute },
				{ "self_deaf", self_deaf },
			}
		}
	})), false);
#endif
	return *this;
}

std::string discord_client::jsonobj_to_string(const nlohmann::json& json) {
	if (protocol == ws_json) {
		return json.dump(-1, ' ', false, json::error_handler_t::replace);
	} else {
		return etf->build(json);
	}
}

void discord_client::disconnect_voice_internal(snowflake guild_id, bool emit_json) {
#ifdef HAVE_VOICE
	std::unique_lock lock(voice_mutex);
	auto v = connecting_voice_channels.find(guild_id);
	if (v != connecting_voice_channels.end()) {
		log(ll_debug, "Disconnecting voice, guild: " + std::to_string(guild_id));
		if (emit_json) {
			queue_message(jsonobj_to_string(json({
				{ "op", ft_voice_state_update },
				{ "d", {
						{ "guild_id", std::to_string(guild_id) },
						{ "channel_id", json::value_t::null },
						{ "self_mute", false },
						{ "self_deaf", false },
					}
				}
			})), false);
		}
		connecting_voice_channels.erase(v);
	}
#endif
}

discord_client& discord_client::disconnect_voice(snowflake guild_id) {
	disconnect_voice_internal(guild_id, true);
	return *this;
}

voiceconn* discord_client::get_voice(snowflake guild_id) {
#ifdef HAVE_VOICE
	std::shared_lock lock(voice_mutex);
	auto v = connecting_voice_channels.find(guild_id);
	if (v != connecting_voice_channels.end()) {
		return v->second.get();
	}
#endif
	return nullptr;
}


voiceconn::voiceconn(discord_client* o, snowflake _channel_id, bool enable_dave) : creator(o), channel_id(_channel_id), voiceclient(nullptr), dave(enable_dave) {
}

bool voiceconn::is_ready() const {
	return (!websocket_hostname.empty() && !session_id.empty() && !token.empty());
}

bool voiceconn::is_active() const {
	return voiceclient != nullptr;
}

voiceconn& voiceconn::disconnect() {
	if (this->is_active()) {
		delete voiceclient;
		voiceclient = nullptr;
	}
	return *this;
}

voiceconn::~voiceconn() {
	this->disconnect();
}

voiceconn& voiceconn::connect(snowflake guild_id) {
	if (this->is_ready() && !this->is_active()) {
		try {
			this->creator->log(ll_debug, "Connecting voice for guild " + std::to_string(guild_id) + " channel " + std::to_string(this->channel_id));
			this->voiceclient = new discord_voice_client(creator->creator, this->channel_id, guild_id, this->token, this->session_id, this->websocket_hostname, this->dave);
			/* Note: Spawns thread! */
			this->voiceclient->run();
		}
		catch (std::exception &e) {
			this->creator->log(ll_debug, "Can't connect to voice websocket (guild_id: " + std::to_string(guild_id) + ", channel_id: " + std::to_string(this->channel_id) + "): " + std::string(e.what()));
		}
	}
	return *this;
}


}
