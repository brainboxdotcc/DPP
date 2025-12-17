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
#include <dpp/channel.h>
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
void channel_pins_update::handle(discord_client* client, json &j, const std::string &raw) {

	if (!client->creator->on_channel_pins_update.empty()) {
		json& d = j["d"];
		channel_pins_update_t cpu(client->owner, client->shard_id, raw);
		snowflake guild_id = snowflake_not_null(&d, "guild_id");
		snowflake channel_id = snowflake_not_null(&d, "channel_id");
		guild* g = find_guild(guild_id);
		channel* c = find_channel(channel_id);

		cpu.pin_channel = c ? *c : channel{};
		cpu.pin_channel.id = channel_id;

		cpu.pin_guild = g ? *g : guild{};
		cpu.pin_guild.id = guild_id;

		cpu.timestamp = ts_not_null(&d, "last_pin_timestamp");

		client->creator->queue_work(0, [c = client->creator, cpu]() {
			c->on_channel_pins_update.call(cpu);
		});
	}

}

};