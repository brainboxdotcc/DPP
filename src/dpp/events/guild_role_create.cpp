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

/**
 * @brief Handle event
 * 
 * @param client Websocket client (current shard)
 * @param j JSON data for the event
 * @param raw Raw JSON string
 */
void guild_role_create::handle(class DiscordClient* client, json &j, const std::string &raw) {
	json &d = j["d"];
	dpp::guild* g = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
	if (g) {
		json &role = d["role"];
		dpp::role *r = dpp::find_role(SnowflakeNotNull(&role, "id"));
		if (!r) {
			r = new dpp::role();
		}
		r->fill_from_json(g->id, &role);
		dpp::get_role_cache()->store(r);
		g->roles.push_back(r->id);
		if (client->creator->dispatch.guild_role_create) {
			dpp::guild_role_create_t grc(raw);
			grc.creating_guild = g;
			grc.created = r;
			client->creator->dispatch.guild_role_create(grc);
		}
	}
}

