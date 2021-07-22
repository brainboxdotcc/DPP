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
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt/format.h>

using json = nlohmann::json;

namespace dpp { namespace events {

using namespace dpp;
void thread_delete::handle(discord_client* client, json& j, const std::string& raw) {
	json& d = j["d"];

	dpp::channel t;
	t.fill_from_json(&d);
	dpp::guild* g = dpp::find_guild(t.guild_id);
	if (g) {
		auto gt = std::find(g->threads.begin(), g->threads.end(), t.id);
		if (gt != g->threads.end()) {
			g->threads.erase(gt);
		}
		if (client->creator->dispatch.thread_delete) {
			dpp::thread_delete_t tc(client, raw);
			tc.deleted = t;
			tc.deleting_guild = g;
			client->creator->dispatch.thread_delete(tc);
		}
	}
}
}};
