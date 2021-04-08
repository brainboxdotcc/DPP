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

void guild_role_update::handle(class DiscordClient* client, json &j, const std::string &raw) {
	json &d = j["d"];
	dpp::guild* g = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
	if (g) {
		json& role = d["role"];
		dpp::role *r = dpp::find_role(SnowflakeNotNull(&role, "id"));
		if (r) {
			r->fill_from_json(g->id, &role);
			if (client->creator->dispatch.guild_role_update) {
				dpp::guild_role_update_t gru(raw);
				gru.updating_guild = g;
				gru.updated = r;
				client->creator->dispatch.guild_role_update(gru);
			}
		}
	}
}

