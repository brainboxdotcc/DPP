#include <dpp/discord.h>
#include <dpp/event.h>
#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <nlohmann/json.hpp>
#include <fmt/format.h>

using json = nlohmann::json;

std::mutex protect_the_loot;

/**
 * @brief Handle event
 * 
 * @param client Websocket client (current shard)
 * @param j JSON data for the event
 * @param raw Raw JSON string
 */
void ready::handle(class DiscordClient* client, json &j, const std::string &raw) {
	client->log(dpp::ll_info, fmt::format("Shard {}/{} ready!", client->shard_id, client->max_shards));
	client->sessionid = j["d"]["session_id"];

	/* Mutex this to make sure multiple threads don't change it at the same time */
	{
		std::lock_guard<std::mutex> lockit(protect_the_loot);
		client->creator->me.fill_from_json(&(j["d"]["user"]));
	}

	if (client->creator->dispatch.ready) {
		dpp::ready_t r(raw);
		r.session_id = client->sessionid;
		r.shard_id = client->shard_id;
		client->creator->dispatch.ready(r);
	}

	/* Simulate GUILD_CREATE for each guild in the list.
	 * These generally arrive as unavailable and are filled by separate GUILD_CREATEs */
	for (auto & g : j["d"]["guilds"]) {
		json gc;
		gc["d"] = g;
		client->HandleEvent("GUILD_CREATE", gc, "");
	}
}

