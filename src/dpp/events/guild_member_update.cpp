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
void guild_member_update::handle(DiscordClient* client, json &j, const std::string &raw) {
       json& d = j["d"];
	dpp::guild* g = dpp::find_guild(from_string<uint64_t>(d["guild_id"].get<std::string>(), std::dec));
	dpp::user* u = dpp::find_user(from_string<uint64_t>(d["user"]["id"].get<std::string>(), std::dec));
	if (g && u) {
		auto& user = d["user"];
		auto gmi = g->members.find(u->id);
		if (gmi != g->members.end()) {
			gmi->second->fill_from_json(&user, g, u);

			if (client->creator->dispatch.guild_member_update) {
				dpp::guild_member_update_t gmu(raw);
				gmu.updating_guild = g;
				gmu.updated = gmi->second;
				client->creator->dispatch.guild_member_update(gmu);
			}
		}
	}
}

}};