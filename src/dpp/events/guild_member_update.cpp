/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2021 Craig Edwards and D++ contributors 
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/
#include <dpp/discord.h>
#include <dpp/event.h>
#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>

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
				dpp::guild_member_update_t gmu(client, raw);
				gmu.updating_guild = g;
				gmu.updated = gmi->second;
				client->creator->dispatch.guild_member_update(gmu);
			}
		}
	}
}

}};