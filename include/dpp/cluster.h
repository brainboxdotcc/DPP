#pragma once

#include <string>
#include <map>
#include <dpp/discord.h>
#include <dpp/dispatcher.h>
#include <spdlog/fwd.h>
#include <dpp/discordclient.h>

namespace dpp {

class cluster {
public:
	dpp::dispatcher dispatch;
	std::map<uint32_t, class DiscordClient*> shards;

	void start(const std::string &token, uint32_t intents = 0, uint32_t shards = 1, uint32_t cluster_id = 0, uint32_t maxclusters = 1, spdlog::logger* log = nullptr);
};

};
