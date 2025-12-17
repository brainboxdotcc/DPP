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
#include <dpp/message.h>
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
void message_reaction_remove_emoji::handle(discord_client* client, json &j, const std::string &raw) {
	if (!client->creator->on_message_reaction_remove_emoji.empty()) {
		json &d = j["d"];
		dpp::message_reaction_remove_emoji_t mrre(client->owner, client->shard_id, raw);
		snowflake guild_id = snowflake_not_null(&d, "guild_id");
		snowflake channel_id = snowflake_not_null(&d, "channel_id");
		guild* g = find_guild(guild_id);
		channel* c = find_channel(channel_id);

		mrre.reacting_guild = g ? *g : guild{};
		mrre.reacting_guild.id = guild_id;

		mrre.channel_id = channel_id;
		mrre.reacting_channel = c ? *c : channel{};
		mrre.reacting_channel.id = channel_id;

		mrre.message_id = snowflake_not_null(&d, "message_id");

		mrre.reacting_emoji = dpp::emoji().fill_from_json(&(d["emoji"]));

		if (mrre.channel_id && mrre.message_id) {
			client->creator->queue_work(1, [c = client->creator, mrre]() {
				c->on_message_reaction_remove_emoji.call(mrre);
			});
		}
	}

}

};