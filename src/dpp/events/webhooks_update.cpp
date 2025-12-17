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
#include <dpp/webhook.h>
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
void webhooks_update::handle(discord_client* client, json &j, const std::string &raw) {
	if (!client->creator->on_webhooks_update.empty()) {
		json& d = j["d"];

		snowflake guild_id = snowflake_not_null(&d, "guild_id");
		snowflake channel_id = snowflake_not_null(&d, "channel_id");
		guild* g = find_guild(guild_id);
		channel* c = find_channel(channel_id);

		dpp::webhooks_update_t wu(client->owner, client->shard_id, raw);

		wu.webhook_guild = g ? *g : guild{};
		wu.webhook_guild.id = guild_id;

		wu.webhook_channel = c ? *c : channel{};
		wu.webhook_channel.id = channel_id;

		client->creator->queue_work(1, [c = client->creator, wu]() {
			c->on_webhooks_update.call(wu);
		});
	}
}

};