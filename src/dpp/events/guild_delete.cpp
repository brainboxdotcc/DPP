#include <dpp/discord.h>
#include <dpp/event.h>
#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discordevents.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void guild_delete::handle(class DiscordClient* client, json &j, const std::string &raw) {
	json& d = j["d"];
	dpp::guild* g = dpp::find_guild(SnowflakeNotNull(&d, "id"));
	if (g) {
		if (!BoolNotNull(&d, "unavailable")) {
			dpp::get_guild_cache()->remove(g);
		} else {
			g->flags |= dpp::g_unavailable;
		}

		if (client->creator->dispatch.guild_delete) {
			dpp::guild_delete_t gd(raw);
			gd.deleted = g;
			client->creator->dispatch.guild_delete(gd);
		}
	}
}

