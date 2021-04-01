#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/cache.h>
#include <spdlog/spdlog.h>
#include <dpp/cluster.h>

DiscordClient::DiscordClient(dpp::cluster* _cluster, uint32_t _shard_id, uint32_t _max_shards, const std::string &_token, uint32_t _intents, spdlog::logger* _logger) : WSClient("gateway.discord.gg", "443"), creator(_cluster), shard_id(_shard_id), max_shards(_max_shards), token(_token), last_heartbeat(time(NULL)), heartbeat_interval(0), last_seq(0), sessionid(""), logger(_logger), intents(_intents)
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
}

void DiscordClient::Run()
{
	do {
		SSLClient::ReadLoop();
		SSLClient::close();
		SSLClient::Connect();
		WSClient::Connect();
	} while(true);
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

				HandleEvent(event, j);
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
	logger->debug("OOF! Error from underlying websocket: {}", errorcode);
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
