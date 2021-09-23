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
#include <string>
#include <iostream>
#include <fstream>
#ifndef WIN32
#include <unistd.h>
#endif
#include <dpp/discordclient.h>
#include <dpp/cache.h>
#include <dpp/cluster.h>
#include <thread>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt/format.h>
#include <zlib.h>

#define PATH_UNCOMPRESSED	"/?v=" DISCORD_API_VERSION "&encoding=json"
#define PATH_COMPRESSED		"/?v=" DISCORD_API_VERSION "&encoding=json&compress=zlib-stream"
#define DECOMP_BUFFER_SIZE	512 * 1024

namespace dpp {

/* This is an internal class, defined externally as just a forward declaration for an opaque pointer.
 * This is because we don't want an external dependency on zlib's headers
 */
class zlibcontext {
public:
	z_stream d_stream;
};

discord_client::discord_client(dpp::cluster* _cluster, uint32_t _shard_id, uint32_t _max_shards, const std::string &_token, uint32_t _intents, bool comp)
       : websocket_client(DEFAULT_GATEWAY, "443", comp ? PATH_COMPRESSED : PATH_UNCOMPRESSED),
	creator(_cluster),
	shard_id(_shard_id),
	max_shards(_max_shards),
	token(_token),
	last_heartbeat(time(NULL)),
	heartbeat_interval(0),
	reconnects(0),
	resumes(0),
	last_seq(0),
	sessionid(""),
	intents(_intents),
	runner(nullptr),
	compressed(comp),
	decompressed_total(0),
	decomp_buffer(nullptr),
	ready(false),
	ping_start(0.0),
	websocket_ping(0.0)
{
	zlib = new zlibcontext();
	Connect();
}

discord_client::~discord_client()
{
	if (runner) {
		runner->join();
		delete runner;
	}
	delete zlib;
}

uint64_t discord_client::get_decompressed_bytes_in()
{
	return decompressed_total;
}

void discord_client::SetupZLib()
{
	if (compressed) {
		zlib->d_stream.zalloc = (alloc_func)0;
		zlib->d_stream.zfree = (free_func)0;
		zlib->d_stream.opaque = (voidpf)0;
		if (inflateInit(&(zlib->d_stream)) != Z_OK) {
			throw dpp::exception("Can't initialise stream compression!");
		}
		this->decomp_buffer = new unsigned char[DECOMP_BUFFER_SIZE];
	}

}

void discord_client::EndZLib()
{
	if (compressed) {
		inflateEnd(&(zlib->d_stream));
		if (this->decomp_buffer) {
			delete[] this->decomp_buffer;
			this->decomp_buffer = nullptr;
		}
	}
}

void discord_client::ThreadRun()
{
	SetupZLib();
	do {
		bool error = false;
		ready = false;
		message_queue.clear();
		ssl_client::read_loop();
		ssl_client::close();
		EndZLib();
		SetupZLib();
		do {
			error = false;
			try {
				ssl_client::Connect();
				websocket_client::Connect();
			}
			catch (const std::exception &e) {
				log(dpp::ll_error, std::string("Error establishing connection, retry in 5 seconds: ") + e.what());
				ssl_client::close();
				std::this_thread::sleep_for(std::chrono::seconds(5));
				error = true;
			}
		} while (error);
	} while(true);
}

void discord_client::Run()
{
	this->runner = new std::thread(&discord_client::ThreadRun, this);
	this->thread_id = runner->native_handle();
}

bool discord_client::HandleFrame(const std::string &buffer)
{
	std::string& data = (std::string&)buffer;

	/* gzip compression is a special case */
	if (compressed) {
		/* Check that we have a complete compressed frame */
		if ((uint8_t)buffer[buffer.size() - 4] == 0x00 && (uint8_t)buffer[buffer.size() - 3] == 0x00 && (uint8_t)buffer[buffer.size() - 2] == 0xFF
		&& (uint8_t)buffer[buffer.size() - 1] == 0xFF) {
			/* Decompress buffer */
			decompressed.clear();
			zlib->d_stream.next_in = (Bytef *)buffer.c_str();
			zlib->d_stream.avail_in = buffer.size();
			do {
				int have = 0;
				zlib->d_stream.next_out = (Bytef*)decomp_buffer;
				zlib->d_stream.avail_out = DECOMP_BUFFER_SIZE;
				int ret = inflate(&(zlib->d_stream), Z_NO_FLUSH);
				have = DECOMP_BUFFER_SIZE - zlib->d_stream.avail_out;
				switch (ret)
				{
					case Z_NEED_DICT:
					case Z_STREAM_ERROR:
						this->Error(6000);
						this->close();
						return true;
					break;
					case Z_DATA_ERROR:
						this->Error(6001);
						this->close();
						return true;
					break;
					case Z_MEM_ERROR:
						this->Error(6002);
						this->close();
						return true;
					break;
					case Z_OK:
						this->decompressed.append((const char*)decomp_buffer, have);
						this->decompressed_total += have;
					break;
					default:
						/* Stub */
					break;
				}
			} while (zlib->d_stream.avail_out == 0);
			data = decompressed;
		} else {
			/* No complete compressed frame yet */
			return false;
		}
	}


	log(dpp::ll_trace, fmt::format("R: {}", data));
	json j;
	
	try {
		j = json::parse(data);
	}
	catch (const std::exception &e) {
		log(dpp::ll_error, fmt::format("discord_client::HandleFrame {} [{}]", e.what(), data));
		return true;
	}

	if (j.find("s") != j.end() && !j["s"].is_null()) {
		last_seq = j["s"].get<uint64_t>();
	}

	if (j.find("op") != j.end()) {
		uint32_t op = j["op"];

		switch (op) {
			case 9:
				/* Reset session state and fall through to 10 */
				op = 10;
				log(dpp::ll_debug, fmt::format("Failed to resume session {}, will reidentify", sessionid));
				this->sessionid = "";
				this->last_seq = 0;
				/* No break here, falls through to state 10 to cause a reidentify */
			case 10:
				/* Need to check carefully for the existence of this before we try to access it! */
				if (j.find("d") != j.end() && j["d"].find("heartbeat_interval") != j["d"].end() && !j["d"]["heartbeat_interval"].is_null()) {
					this->heartbeat_interval = j["d"]["heartbeat_interval"].get<uint32_t>();
				}

				if (last_seq && !sessionid.empty()) {
					/* Resume */
					log(dpp::ll_debug, fmt::format("Resuming session {} with seq={}", sessionid, last_seq));
					json obj = {
						{ "op", 6 },
						{ "d", {
								{"token", this->token },
								{"session_id", this->sessionid },
								{"seq", this->last_seq }
							}
						}
					};
					this->write(obj.dump());
					resumes++;
				} else {
					/* Full connect */
					while (time(NULL) < creator->last_identify + 5) {
						uint32_t wait = (creator->last_identify + 5) - time(NULL);
						log(dpp::ll_debug, fmt::format("Waiting {} seconds before identifying for session...", wait));
						std::this_thread::sleep_for(std::chrono::seconds(wait));
					}
					log(dpp::ll_debug, "Connecting new session...");
						json obj = {
						{ "op", 2 },
						{
							"d",
							{
								{ "token", this->token },
								{ "properties",
									{
										{ "$os", "Linux" },
										{ "$browser", "D++" },
										{ "$device", "D++" }
									}
								},
								{ "shard", json::array({ shard_id, max_shards }) },
								{ "compress", false },
								{ "large_threshold", 250 }
							}
						}
					};
					if (this->intents) {
						obj["d"]["intents"] = this->intents;
					}
					this->write(obj.dump());
					this->connect_time = creator->last_identify = time(NULL);
					reconnects++;
				}
				this->last_heartbeat_ack = time(nullptr);
				websocket_ping = 0;
			break;
			case 0: {
				std::string event = j.find("t") != j.end() && !j["t"].is_null() ? j["t"] : "";

				HandleEvent(event, j, data);
			}
			break;
			case 7:
				log(dpp::ll_debug, fmt::format("Reconnection requested, closing socket {}", sessionid));
				message_queue.clear();

				shutdown(sfd, 2);
			#ifdef _WIN32
				if (sfd >= 0 && sfd < FD_SETSIZE) {
					closesocket(sfd);
				}
			#else
				::close(sfd);
			#endif

			break;
			/* Heartbeat ack */
			case 11:
				this->last_heartbeat_ack = time(nullptr);
				websocket_ping = utility::time_f() - ping_start;
			break;
		}
	}
	return true;
}

dpp::utility::uptime discord_client::get_uptime()
{
	return dpp::utility::uptime(time(NULL) - connect_time);
}

bool discord_client::is_connected()
{
	return (this->GetState() == CONNECTED) && (this->ready);
}

void discord_client::Error(uint32_t errorcode)
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
	log(dpp::ll_warning, fmt::format("OOF! Error from underlying websocket: {}: {}", errorcode, error));
}

void discord_client::log(dpp::loglevel severity, const std::string &msg) const
{
	if (creator->dispatch.log) {
		/* Pass to user if theyve hooked the event */
		dpp::log_t logmsg(nullptr, msg);
		logmsg.severity = severity;
		logmsg.message = msg;
		creator->dispatch.log(logmsg);
	}
}

void discord_client::QueueMessage(const std::string &j, bool to_front)
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	if (to_front) {
		message_queue.push_front(j);
	} else {
		message_queue.push_back(j);
	}
}

void discord_client::ClearQueue()
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	message_queue.clear();
}

size_t discord_client::GetQueueSize()
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	return message_queue.size();
}

void discord_client::one_second_timer()
{

	websocket_client::one_second_timer();

	/* Every minute, rehash all containers from first shard.
	 * We can't just get shard with the id 0 because this won't
	 * work on a clustered environment
	 */
	auto shards = creator->get_shards();
	auto first_iter = shards.begin();
	if (first_iter != shards.end()) {
		dpp::discord_client* first_shard = first_iter->second;
		if ((time(NULL) % 60) == 0 && first_shard == this) {
			dpp::garbage_collection();
		}
	}

	/* This all only triggers if we are connected (have completed websocket, and received READY or RESUMED) */
	if (this->is_connected()) {

		/* If we stopped getting heartbeat acknowledgements, this means the connections is dead.
		 * This can happen to TCP connections which is why we have heartbeats in the first place.
		 * Miss two ACKS, forces a reconnection.
		 */
		if ((time(nullptr) - this->last_heartbeat_ack) > heartbeat_interval * 2) {
			log(dpp::ll_warning, fmt::format("Missed heartbeat ACK, forcing reconnection to session {}", sessionid));
			message_queue.clear();

		shutdown(sfd, 2);
		#ifdef _WIN32
			if (sfd >= 0 && sfd < FD_SETSIZE) {
				closesocket(sfd);
			}
		#else
			::close(sfd);
		#endif
			
			return;
		}

		/* Rate limit outbound messages, 1 every odd second, 2 every even second */
		for (int x = 0; x < (time(NULL) % 2) + 1; ++x) {
			std::lock_guard<std::mutex> locker(queue_mutex);
			if (message_queue.size()) {
				std::string message = message_queue.front();
				message_queue.pop_front();
				/* Checking here with .find() saves us having to deserialise the json
				 * to find pings in our queue. The assumption is that the format of the
				 * ping isn't going to change.
				 */
				if (message.find("\"op\":1}") != std::string::npos) {
					ping_start = utility::time_f();
				}
				this->write(message);
			}
		}

		/* Send pings (heartbeat opcodes) before each interval. We send them slightly more regular than expected,
		 * just to be safe.
		 */
		if (this->heartbeat_interval && this->last_seq) {
			/* Check if we're due to emit a heartbeat */
			if (time(NULL) > last_heartbeat + ((heartbeat_interval / 1000.0) * 0.75)) {
				QueueMessage(json({{"op", 1}, {"d", last_seq}}).dump(), true);
				last_heartbeat = time(NULL);
			}
		}
	}
}

uint64_t discord_client::get_guild_count() {
	uint64_t total = 0;
	dpp::cache* c = dpp::get_guild_cache();
	dpp::cache_container& gc = c->get_container();
	/* IMPORTANT: We must lock the container to iterate it */
	std::lock_guard<std::mutex> lock(c->get_mutex());
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
	dpp::cache* c = dpp::get_guild_cache();
	dpp::cache_container& gc = c->get_container();
	/* IMPORTANT: We must lock the container to iterate it */
	std::lock_guard<std::mutex> lock(c->get_mutex());
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
	dpp::cache* c = dpp::get_guild_cache();
	dpp::cache_container& gc = c->get_container();
	/* IMPORTANT: We must lock the container to iterate it */
	std::lock_guard<std::mutex> lock(c->get_mutex());
	for (auto g = gc.begin(); g != gc.end(); ++g) {
		dpp::guild* gp = (dpp::guild*)g->second;
		if (gp->shard_id == this->shard_id) {
			total += gp->channels.size();
		}
	}
	return total;
}

void discord_client::connect_voice(snowflake guild_id, snowflake channel_id) {
#ifdef HAVE_VOICE
	std::lock_guard<std::mutex> lock(voice_mutex);
	if (connecting_voice_channels.find(guild_id) == connecting_voice_channels.end()) {
		connecting_voice_channels[guild_id] = new voiceconn(this, channel_id);
		/* Once sent, this expects two events (in any order) on the websocket:
		* VOICE_SERVER_UPDATE and VOICE_STATUS_UPDATE
		*/
		log(ll_debug, fmt::format("Sending op 4, guild {}", guild_id));
		QueueMessage(json({
			{ "op", 4 },
			{ "d", {
					{ "guild_id", std::to_string(guild_id) },
					{ "channel_id", std::to_string(channel_id) },
					{ "self_mute", false },
					{ "self_deaf", false },
				}
			}
		}).dump(), false);
	}
#endif
}

void discord_client::disconnect_voice(snowflake guild_id) {
#ifdef HAVE_VOICE
	std::lock_guard<std::mutex> lock(voice_mutex);
	auto v = connecting_voice_channels.find(guild_id);
	if (v != connecting_voice_channels.end()) {
		log(ll_debug, fmt::format("Disconnecting voice, guild: {}", guild_id));
		QueueMessage(json({
			{ "op", 4 },
			{ "d", {
					{ "guild_id", std::to_string(guild_id) },
					{ "channel_id", json::value_t::null },
					{ "self_mute", false },
					{ "self_deaf", false },
				}
			}
		}).dump(), false);
		delete v->second;
		v->second = nullptr;
		connecting_voice_channels.erase(v);
	}
#endif
}

voiceconn* discord_client::get_voice(snowflake guild_id) {
#ifdef HAVE_VOICE
	std::lock_guard<std::mutex> lock(voice_mutex);
	auto v = connecting_voice_channels.find(guild_id);
	if (v != connecting_voice_channels.end()) {
		return v->second;
	}
#endif
	return nullptr;
}


voiceconn::voiceconn(discord_client* o, snowflake _channel_id) : creator(o), channel_id(_channel_id), voiceclient(nullptr) {
}

bool voiceconn::is_ready() {
	return (!websocket_hostname.empty() && !session_id.empty() && !token.empty());
}

bool voiceconn::is_active() {
	return voiceclient != nullptr;
}

void voiceconn::disconnect() {
	if (this->is_active()) {
		voiceclient->terminating = true;
		voiceclient->close();
		delete voiceclient;
		voiceclient = nullptr;
	}
}

voiceconn::~voiceconn() {
	this->disconnect();
}

void voiceconn::connect(snowflake guild_id) {
	if (this->is_ready() && !this->is_active()) {
		/* This is wrapped in a thread because instantiating discord_voice_client can initiate a blocking SSL_connect() */
		auto t = std::thread([guild_id, this]() {
			try {
				this->creator->log(ll_debug, fmt::format("Connecting voice for guild {} channel {}", guild_id, this->channel_id));
				this->voiceclient = new discord_voice_client(creator->creator, this->channel_id, guild_id, this->token, this->session_id, this->websocket_hostname);
				/* Note: Spawns thread! */
				this->voiceclient->Run();
			}
			catch (std::exception &e) {
				this->creator->log(ll_error, fmt::format("Can't connect to voice websocket (guild_id: {}, channel_id: {}): {}", guild_id, this->channel_id, e.what()));
			}
		});
		t.detach();
	}
}


};