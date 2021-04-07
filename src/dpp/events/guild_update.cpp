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

void guild_update::handle(class DiscordClient* client, json &j) {
       json& d = j["d"];
	dpp::guild* g = dpp::find_guild(from_string<uint64_t>(d["id"].get<std::string>(), std::dec));
	if (g) {
		g->fill_from_json(&d);
		if (!g->is_unavailable()) {
			for (int rc = 0; rc < g->roles.size(); ++rc) {
				dpp::role* oldrole = dpp::find_role(g->roles[rc]);
				dpp::get_role_cache()->remove(oldrole);
			}
			g->roles.clear();
			for (auto & role : d["roles"]) {
				dpp::role *r = new dpp::role();
				r->fill_from_json(g->id, &role);
				dpp::get_role_cache()->store(r);
				g->roles.push_back(r->id);
			}
		}

		if (client->creator->dispatch.guild_update) {
			dpp::guild_update_t gu(d.dump());
			gu.updated = g;
			client->creator->dispatch.guild_update(gu);
		}
	}
}

