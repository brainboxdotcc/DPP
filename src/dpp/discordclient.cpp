#include <string>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <resolv.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <dpp/discordclient.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

DiscordClient::DiscordClient(uint32_t _shard_id, uint32_t _max_shards, const std::string &_token) : WSClient("gateway.discord.gg", "443"), shard_id(_shard_id), max_shards(_max_shards), token(_token), last_heartbeat(time(NULL)), heartbeat_interval(0), last_seq(0), sessionid("")
{
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
	//std::cout << "R: " << buffer << "\n";
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
				std::cout << "Failed to resume session " << sessionid << ", will reidentify\n";
				this->sessionid = "";
				this->last_seq = 0;
				/* No break here, falls through to state 10 to cause a reidentify */
			case 10:
				this->heartbeat_interval = j["d"]["heartbeat_interval"].get<uint32_t>();

				if (last_seq && !sessionid.empty()) {
					/* Resume */
					std::cout << "Resuming session " << sessionid << " with seq=" << last_seq << "...\n";
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
					std::cout << "Connecting new session...\n";
				    	json obj = {
					    { "op", 2 },
					    {
						"d",
						{
						    { "token", this->token },
						    { "intents", 513 },
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
					this->write(obj.dump());
				}
			break;
			case 0:
				std::string event = j.find("t") != j.end() && !j["t"].is_null() ? j["t"] : "";

				if (event == "READY") {
					this->sessionid = j["d"]["session_id"];
					std::cout << "Received READY, session: " << sessionid << "\n";
				} else if (event == "RESUMED") {
					std::cout << "Successfully resumed session id " << sessionid << "\n";
				} else if (event == "RECONNECT") {
					std::cout << "Reconnection requested, closing socket\n" << sessionid << "\n";
					::close(sfd);
				}
			break;
		}
	}
	return true;
}

void DiscordClient::Error(uint32_t errorcode)
{
	std::cout << "OOF! Error from underlying websocket: " << errorcode << "\n";
}

void DiscordClient::OneSecondTimer()
{
	if (this->heartbeat_interval) {
		/* Check if we're due to emit a heartbeat */
		if (time(NULL) > last_heartbeat + (heartbeat_interval / 1000.0) - 2) {
			std::cout << "Emit heartbeat, seq=" << last_seq << "\n";
			this->write(json({{"op", 1}, {"d", last_seq}}).dump());
			last_heartbeat = time(NULL);
		}
	}
}
