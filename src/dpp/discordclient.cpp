#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

DiscordClient::DiscordClient(uint32_t _shard_id, uint32_t _max_shards, const std::string &_token) : WSClient("gateway.discord.gg", "443"), shard_id(_shard_id), max_shards(_max_shards), token(_token)
{
}

DiscordClient::~DiscordClient()
{
}

bool DiscordClient::HandleFrame(const std::string &buffer)
{
	std::cout << "R: " << buffer << "\n";
	json j = json::parse(buffer);
	if (j.find("op") != j.end()) {
		uint32_t op = j["op"];

		switch (op) {
			case 10:
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
				this->heartbeat_interval = j["d"]["heartbeat_interval"].get<uint32_t>();
				this->write(obj.dump());
			break;
		}
	}
	return true;
}

void DiscordClient::Error(uint32_t errorcode)
{
	std::cout << "OOF! Error from underlying websocket: " << errorcode << "\n";
}
