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
void thread_list_sync::handle(discord_client* client, json& j, const std::string& raw) {
	json& d = j["d"];

	dpp::guild* g = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
	if (g) {
		if (d.find("threads") != d.end()) {
			for (auto& t : d["threads"]) {
				g->threads.push_back(SnowflakeNotNull(&t, "id"));
			}
		}
		if (client->creator->dispatch.thread_list_sync) {
			dpp::thread_list_sync_t tls(client, raw);
			if (d.find("threads") != d.end()) {
				for (auto& t : d["threads"]) {
					tls.threads.push_back(channel().fill_from_json(&t));
				}
			}
			if (d.find("members") != d.end()) {
				for (auto& tm : d["members"]) {
					tls.members.push_back(thread_member().fill_from_json(&tm));
				}
			}
			client->creator->dispatch.thread_list_sync(tls);
		}
	}
}
}};
