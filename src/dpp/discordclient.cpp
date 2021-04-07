#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/cache.h>
#include <spdlog/spdlog.h>
#include <dpp/cluster.h>
#include <thread>

DiscordClient::DiscordClient(dpp::cluster* _cluster, uint32_t _shard_id, uint32_t _max_shards, const std::string &_token, uint32_t _intents, spdlog::logger* _logger) : WSClient("gateway.discord.gg", "443"), creator(_cluster), shard_id(_shard_id), max_shards(_max_shards), token(_token), last_heartbeat(time(NULL)), heartbeat_interval(0), last_seq(0), sessionid(""), logger(_logger), intents(_intents), runner(nullptr)
{
	if (logger == nullptr) {
		try {
			std::shared_ptr<spdlog::logger> log;
			std::vector<spdlog::sink_ptr> sinks;
			log = std::make_shared<spdlog::logger>("nullsink", begin(sinks), end(sinks));
			spdlog::register_logger(log);
			logger = log.get();
		}
		catch (const spdlog::spdlog_ex& ex) {
			std::cout << "Log initialization failed: " << ex.what() << std::endl;
		}
	}
}

DiscordClient::~DiscordClient()
{
	if (runner) {
		runner->join();
		delete runner;
	}
}

void DiscordClient::ThreadRun()
{
	do {
		SSLClient::ReadLoop();
		SSLClient::close();
		SSLClient::Connect();
		WSClient::Connect();
	} while(true);
}

void DiscordClient::Run()
{
	runner = new std::thread(&DiscordClient::ThreadRun, this);
}

bool DiscordClient::HandleFrame(const std::string &buffer)
{
	logger->trace("R: {}", buffer);
	json j = json::parse(buffer);

	if (j.find("s") != j.end() && !j["s"].is_null()) {
		last_seq = j["s"].get<uint64_t>();
	}

	if (j.find("op") != j.end()) {
		uint32_t op = j["op"];

		switch (op) {
			case 9:
				/* Reset session state and fall through to 9 */
				op = 10;
				logger->debug("Failed to resume session {}, will reidentify", sessionid);
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
					logger->debug("Resuming session {} with seq={}", sessionid, last_seq);
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
					logger->debug("Connecting new session...");
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
				}
			break;
			case 0: {
				std::string event = j.find("t") != j.end() && !j["t"].is_null() ? j["t"] : "";

				HandleEvent(event, j, buffer);
			}
			break;
			case 7:
				logger->debug("Reconnection requested, closing socket {}", sessionid);
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
		{ 6666, "Hell freezing over" }
	};
	std::string error = "Unknown error";
	auto i = errortext.find(errorcode);
	if (i != errortext.end()) {
		error = i->second;
	}
	logger->debug("OOF! Error from underlying websocket: {}: {}", errorcode, error);
}

void DiscordClient::OneSecondTimer()
{
	if (this->GetState() == CONNECTED) {
		if (this->heartbeat_interval) {
			/* Check if we're due to emit a heartbeat */
			if (time(NULL) > last_heartbeat + ((heartbeat_interval / 1000.0) * 0.75)) {
				logger->debug("Emit heartbeat, seq={}", last_seq);
				this->write(json({{"op", 1}, {"d", last_seq}}).dump());
				last_heartbeat = time(NULL);
				dpp::garbage_collection();
			}
		}
		/* Rate limited chunk requests, 1 every odd second, 2 every even second */
		for (int x = 0; x < (time(NULL) % 2) + 1; ++x) {
			if (chunk_queue.size()) {
				uint64_t next_guild_chunk = chunk_queue.front();
				chunk_queue.pop();
				json chunk_req = json({{"op", 8}, {"d", {{"guild_id",std::to_string(next_guild_chunk)},{"query",""},{"limit",0}}}});
				if (this->intents & dpp::GUILD_PRESENCES) {
					chunk_req["d"]["presences"] = true;
				}
				this->write(chunk_req.dump());
			}
		}
	}
}
