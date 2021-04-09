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
void guild_member_add::handle(class DiscordClient* client, json &j, const std::string &raw) {
	json d = j["d"];

	dpp::guild* g = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
	if (g) {
		dpp::user* u = dpp::find_user(SnowflakeNotNull(&(d["user"]), "id"));
		if (!u) {
			u = new dpp::user();
			u->fill_from_json(&(d["user"]));
			dpp::get_user_cache()->store(u);
		}
		dpp::guild_member* gm = new dpp::guild_member();
		gm->fill_from_json(&d, g, u);
		g->members[u->id] = gm;

		if (client->creator->dispatch.guild_member_add) {
			dpp::guild_member_add_t gmr(raw);
			gmr.adding_guild = g;
			gmr.added = gm;
			client->creator->dispatch.guild_member_add(gmr);
		}
	}
}

