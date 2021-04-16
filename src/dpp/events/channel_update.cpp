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

using json = nlohmann::json;

namespace dpp { namespace events {

using namespace dpp;

/**
 * @brief Handle event
 * 
 * @param client Websocket client (current shard)
 * @param j JSON data for the event
 * @param raw Raw JSON string
 */
void channel_update::handle(DiscordClient* client, json &j, const std::string &raw) {
	json& d = j["d"];
	dpp::channel* c = dpp::find_channel(from_string<uint64_t>(d["id"].get<std::string>(), std::dec));
	if (c) {
		c->fill_from_json(&d);
		if (client->creator->dispatch.channel_update) {
			dpp::channel_update_t cu(client, raw);
	  		cu.updated = c;
  			cu.updating_guild = dpp::find_guild(c->guild_id);
			client->creator->dispatch.channel_update(cu);
		}
	}
}

}};