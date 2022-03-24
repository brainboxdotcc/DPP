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
#include <dpp/discordevents.h>
#include <dpp/cluster.h>
#include <dpp/channel.h>
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
void channel_delete::handle(discord_client* client, json &j, const std::string &raw) {
	json& d = j["d"];
	dpp::channel* c = dpp::find_channel(snowflake_not_null(&d, "id"));
	if (c) {
		dpp::guild* g = dpp::find_guild(c->guild_id);
		if (g) {
			auto gc = std::find(g->channels.begin(), g->channels.end(), c->id);
			if (gc != g->channels.end()) {
				g->channels.erase(gc);
			}

			if (!client->creator->on_channel_delete.empty()) {
				dpp::channel_delete_t cd(client, raw);
				cd.deleted = c;
				cd.deleting_guild = g;
				client->creator->on_channel_delete.call(cd);
			}
		}
		dpp::get_channel_cache()->remove(c);
	}
}

}};