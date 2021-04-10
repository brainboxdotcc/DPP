#include <string>
#include <iostream>
#include <fstream>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <dpp/discordclient.h>
#include <dpp/cache.h>
#include <dpp/cluster.h>
#include <thread>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <zlib.h>

#define DEFAULT_GATEWAY		"gateway.discord.gg"
#define PATH_UNCOMPRESSED	"/?v=6&encoding=json"
#define PATH_COMPRESSED		"/?v=6&encoding=json&compress=zlib-stream"
#define DECOMP_BUFFER_SIZE	512 * 1024

DiscordClient::DiscordClient(dpp::cluster* _cluster, uint32_t _shard_id, uint32_t _max_shards, const std::string &_token, uint32_t _intents, bool comp)
       : WSClient(DEFAULT_GATEWAY, "443", comp ? PATH_COMPRESSED : PATH_UNCOMPRESSED),
	creator(_cluster),
	shard_id(_shard_id),
	max_shards(_max_shards),
	token(_token),
	last_heartbeat(time(NULL)),
	heartbeat_interval(0),
	last_seq(0),
	sessionid(""),
	intents(_intents),
	runner(nullptr),
	compressed(comp),
	decompressed_total(0),
	decomp_buffer(nullptr)
{
	Connect();
}

DiscordClient::~DiscordClient()
{
	if (runner) {
		runner->join();
		delete runner;
	}
}

uint64_t DiscordClient::GetDeompressedBytesIn()
{
	return 0;
}

void DiscordClient::SetupZLib()
{
	if (compressed) {
		d_stream.zalloc = (alloc_func)0;
		d_stream.zfree = (free_func)0;
		d_stream.opaque = (voidpf)0;
		if (inflateInit(&d_stream) != Z_OK) {
			throw std::runtime_error("Can't initialise stream compression!");
		}
		this->decomp_buffer = new unsigned char[DECOMP_BUFFER_SIZE];
		log(dpp::ll_debug, fmt::format("Starting compression of shard {}", shard_id));
	}

}

void DiscordClient::EndZLib()
{
	if (compressed) {
		inflateEnd(&d_stream);
		if (this->decomp_buffer) {
			delete[] this->decomp_buffer;
			this->decomp_buffer = nullptr;
		}
	}
}

void DiscordClient::ThreadRun()
{
	SetupZLib();
	do {
		SSLClient::ReadLoop();
		SSLClient::close();
		EndZLib();
		SetupZLib();
		SSLClient::Connect();
		WSClient::Connect();
	} while(true);
}

void DiscordClient::Run()
{
	this->runner = new std::thread(&DiscordClient::ThreadRun, this);
	this->thread_id = runner->native_handle();
}

bool DiscordClient::HandleFrame(const std::string &buffer)
{
	std::string& data = (std::string&)buffer;

	/* gzip compression is a special case */
	if (compressed) {
		/* Check that we have a complete compressed frame */
		if ((uint8_t)buffer[buffer.size() - 4] == 0x00 && (uint8_t)buffer[buffer.size() - 3] == 0x00 && (uint8_t)buffer[buffer.size() - 2] == 0xFF
		&& (uint8_t)buffer[buffer.size() - 1] == 0xFF) {
			/* Decompress buffer */
			decompressed.clear();
			d_stream.next_in = (Bytef *)buffer.c_str();
			d_stream.avail_in = buffer.size();
			do {
				int have = 0;
				d_stream.next_out = (Bytef*)decomp_buffer;
				d_stream.avail_out = DECOMP_BUFFER_SIZE;
				int ret = inflate(&d_stream, Z_NO_FLUSH);
				have = DECOMP_BUFFER_SIZE - d_stream.avail_out;
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
			} while (d_stream.avail_out == 0);
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
		log(dpp::ll_error, fmt::format("DiscordClient::HandleFrame {} [{}]", e.what(), data));
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
					creator->last_identify = time(NULL);
				}
			break;
			case 0: {
				std::string event = j.find("t") != j.end() && !j["t"].is_null() ? j["t"] : "";

				HandleEvent(event, j, data);
			}
			break;
			case 7:
				log(dpp::ll_debug, fmt::format("Reconnection requested, closing socket {}", sessionid));
				::close(sfd);
			break;
		}
	}
	return true;
}

void DiscordClient::Error(uint32_t errorcode)
{
	std::map<uint32_t, std::string> errortext = {
		{ 1000, "Socket shutdown" },
		{ 1001, "Client is leaving" },
		{ 1002, "Endpoint received a malformed frame" },
		{ 1003, "Endpoint received an unsupported frame" },
		{ 1004, "Reserved code" },
		{ 1005, "Expected close status, received none" },
		{ 1006, "No close code frame has been receieved" },
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

void DiscordClient::log(dpp::loglevel severity, const std::string &msg)
{
	creator->log(severity, msg);
}

void DiscordClient::QueueMessage(const std::string &j, bool to_front)
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	if (to_front) {
		message_queue.push_front(j);
	} else {
		message_queue.push_back(j);
	}
}

void DiscordClient::ClearQueue()
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	message_queue.clear();
}

size_t DiscordClient::GetQueueSize()
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	return message_queue.size();
}

void DiscordClient::OneSecondTimer()
{
	/* Rate limit outbound messages, 1 every odd second, 2 every even second */
	if (this->GetState() == CONNECTED) {
		for (int x = 0; x < (time(NULL) % 2) + 1; ++x) {
			if (message_queue.size()) {
				std::lock_guard<std::mutex> locker(queue_mutex);
				std::string message = message_queue.front();
				message_queue.pop_front();
				this->write(message);
			}
		}

		if (this->heartbeat_interval && this->last_seq) {
			/* Check if we're due to emit a heartbeat */
			if (time(NULL) > last_heartbeat + ((heartbeat_interval / 1000.0) * 0.75)) {
				log(dpp::ll_debug, fmt::format("Emit heartbeat, seq={}", last_seq));
				QueueMessage(json({{"op", 1}, {"d", last_seq}}).dump(), true);
				last_heartbeat = time(NULL);
				dpp::garbage_collection();
			}
		}
	}
}
