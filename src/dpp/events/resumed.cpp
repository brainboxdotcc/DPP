#include <dpp/discord.h>
#include <dpp/event.h>
#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#define SPDLOG_FMT_EXTERNAL
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void resumed::handle(class DiscordClient* client, json &j, const std::string &raw) {
	client->logger->debug("Successfully resumed session id {}", client->sessionid);

	if (client->creator->dispatch.resumed) {
		dpp::resumed_t r(raw);
		r.session_id = client->sessionid;
		r.shard_id = client->shard_id;
		client->creator->dispatch.resumed(r);
	}
}

