#include <dpp/discord.h>
#include <dpp/event.h>
#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void ready::handle(class DiscordClient* client, json &j, const std::string &raw) {
	client->logger->info("Shard {}/{} ready!", client->shard_id, client->max_shards);
	client->sessionid = j["d"]["session_id"];

	if (client->creator->dispatch.ready) {
		dpp::ready_t r(raw);
		r.session_id = client->sessionid;
		r.shard_id = client->shard_id;
		client->creator->dispatch.ready(r);
	}
}

