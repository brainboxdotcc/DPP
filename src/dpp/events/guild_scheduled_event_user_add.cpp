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
void guild_scheduled_event_user_add::handle(discord_client* client, json &j, const std::string &raw) {
	json& d = j["d"];
	if (!client->creator->on_guild_scheduled_event_user_add.empty()) {
		dpp::guild_scheduled_event_user_add_t eua(client, raw);
		eua.guild_id = snowflake_not_null(&d, "guild_id");
		eua.user_id = snowflake_not_null(&d, "user_id");
		eua.event_id = snowflake_not_null(&d, "guild_scheduled_event_id");
		client->creator->on_guild_scheduled_event_user_add.call(eua);
	}
}

}};