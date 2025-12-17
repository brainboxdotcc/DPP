/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <dpp/json.h>



namespace dpp::events {
/**
 * @brief Handle event
 * 
 * @param client Websocket client (current shard)
 * @param j JSON data for the event
 * @param raw Raw JSON string
 */
void typing_start::handle(discord_client* client, json &j, const std::string &raw) {
	if (!client->creator->on_typing_start.empty()) {
		json& d = j["d"];
		snowflake guild_id = snowflake_not_null(&d, "guild_id");
		snowflake channel_id = snowflake_not_null(&d, "channel_id");
		snowflake user_id = snowflake_not_null(&d, "user_id");
		guild* g = find_guild(guild_id);
		channel* c = find_channel(channel_id);
		user* u = find_user(user_id);

		dpp::typing_start_t ts(client->owner, client->shard_id, raw);
		ts.typing_guild = g ? *g : guild{};
		ts.typing_guild.id = guild_id;

		ts.typing_channel = c ? *c : channel{};
		ts.typing_channel.id = channel_id;

		ts.user_id = user_id;
		ts.typing_user = u ? *u : user{};
		ts.typing_user.id = user_id;

		ts.timestamp = ts_not_null(&d, "timestamp");
		client->creator->queue_work(1, [c = client->creator, ts]() {
			c->on_typing_start.call(ts);
		});
	}
}

};