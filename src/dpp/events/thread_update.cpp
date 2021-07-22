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
void thread_update::handle(discord_client* client, json& j, const std::string& raw) {
	json& d = j["d"];

	t->fill_from_json(&d);
	if (client->creator->dispatch.thread_update) {
		dpp::thread_update_t tc(client, raw);
		tc.updated = t;
		tc.updating_guild = g;
		client->creator->dispatch.thread_update(tc);
	}
}
}};
