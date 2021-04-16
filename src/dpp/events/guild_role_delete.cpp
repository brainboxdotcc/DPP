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
void guild_role_delete::handle(DiscordClient* client, json &j, const std::string &raw) {
	json &d = j["d"];
	dpp::guild* g = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
	if (g) {
		json& role = d["role"];
		dpp::role *r = dpp::find_role(SnowflakeNotNull(&role, "id"));
		if (r) {
			if (client->creator->dispatch.guild_role_delete) {
				dpp::guild_role_delete_t grd(client, raw);
				grd.deleting_guild = g;
				grd.deleted = r;
				client->creator->dispatch.guild_role_delete(grd);
			}
			auto i = std::find(g->roles.begin(), g->roles.end(), r->id);
			if (i != g->roles.end()) {
				g->roles.erase(i);
			}
			dpp::get_role_cache()->remove(r);
		}
	}
}

}};