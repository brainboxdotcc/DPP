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
#include <dpp/discordevents.h>

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
void guild_role_update::handle(DiscordClient* client, json &j, const std::string &raw) {
	json &d = j["d"];
	dpp::guild* g = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
	if (g) {
		json& role = d["role"];
		dpp::role *r = dpp::find_role(SnowflakeNotNull(&role, "id"));
		if (r) {
			r->fill_from_json(g->id, &role);
			if (client->creator->dispatch.guild_role_update) {
				dpp::guild_role_update_t gru(client, raw);
				gru.updating_guild = g;
				gru.updated = r;
				client->creator->dispatch.guild_role_update(gru);
			}
		}
	}
}

}};