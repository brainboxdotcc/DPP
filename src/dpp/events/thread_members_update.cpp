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
void thread_members_update::handle(discord_client* client, json& j, const std::string& raw) {
	json& d = j["d"];

	dpp::guild* g = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
	if (g) {
		if (client->creator->dispatch.thread_members_update) {
			dpp::thread_members_update_t tms(client, raw);
			tms.updating_guild = g;
			SetSnowflakeNotNull(&d, "id", tms.thread_id);
			SetInt8NotNull(&d, "member_count", tms.member_count);
			if (d.find("added_members") != d.end()) {
				for (auto& tm : d["added_members"]) {
					tms.added.push_back(thread_member().fill_from_json(&tm));
				}
			}
			if (d.find("removed_member_ids") != d.end()) {
				try {
					for (auto& rm : d["removed_member_ids"]) {
						tms.removed_ids.push_back(std::stoull(static_cast<std::string>(rm)));
					}
				} catch (const std::exception& e) {
					client->creator->log(dpp::ll_error, fmt::format("thread_members_update: {}", e.what()));
				}
			}
			client->creator->dispatch.thread_members_update(tms);
		}
	}
}
}};
