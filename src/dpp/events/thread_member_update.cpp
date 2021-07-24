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
void thread_member_update::handle(discord_client* client, json& j, const std::string& raw) {
	json& d = j["d"];
	if (client->creator->dispatch.thread_member_update) {
		dpp::thread_member_update_t tm(client, raw);
		tm.updated = thread_member().fill_from_json(&j);

		client->creator->dispatch.thread_member_update(tm);
	}
}
}};
