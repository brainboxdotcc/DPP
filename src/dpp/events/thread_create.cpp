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
void thread_create::handle(discord_client* client, json& j, const std::string& raw) {
	json& d = j["d"];

	dpp::channel t;
	t.fill_from_json(&d);
	dpp::guild* g = dpp::find_guild(t.guild_id);
	if (g) {
		g->threads.push_back(t.id);
		if (client->creator->dispatch.thread_create) {
			dpp::thread_create_t tc(client, raw);
			tc.created = t;
			tc.creating_guild = g;
			client->creator->dispatch.thread_create(tc);
		}
	}
}
}};
