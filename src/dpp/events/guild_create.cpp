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
#include <dpp/discordevents.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <nlohmann/json.hpp>

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
void guild_create::handle(DiscordClient* client, json &j, const std::string &raw) {
	json& d = j["d"];
	bool newguild = false;
	if (SnowflakeNotNull(&d, "id") == 0)
		return;
	dpp::guild* g = dpp::find_guild(SnowflakeNotNull(&d, "id"));
	if (!g) {
		g = new dpp::guild();
		newguild = true;
	}
	g->fill_from_json(client, &d);
	g->shard_id = client->shard_id;
	if (!g->is_unavailable()) {
		/* Store guild roles */
		g->roles.clear();
		g->roles.reserve(d["roles"].size());
		for (auto & role : d["roles"]) {
			dpp::role *r = dpp::find_role(SnowflakeNotNull(&role, "id"));
			if (!r) {
				r = new dpp::role();
			}
			r->fill_from_json(g->id, &role);
			dpp::get_role_cache()->store(r);
			g->roles.push_back(r->id);
		}

		/* Store guild channels */
		g->channels.clear();
		g->channels.reserve(d["channels"].size());
		for (auto & channel : d["channels"]) {
			dpp::channel* c = dpp::find_channel(SnowflakeNotNull(&channel, "id"));
			if (!c) {
				c = new dpp::channel();
			}
			c->fill_from_json(&channel);
			c->guild_id = g->id;
			dpp::get_channel_cache()->store(c);
			g->channels.push_back(c->id);
		}

		/* Store guild members */
		g->members->reserve(d["members"].size());
		for (auto & user : d["members"]) {
			snowflake userid = SnowflakeNotNull(&(user["user"]), "id");
			/* Only store ones we don't have already otherwise gm will leak */
			if (g->members->find(userid) == g->members->end()) {
				dpp::user* u = dpp::find_user(userid);
				if (!u) {
					u = new dpp::user();
					u->fill_from_json(&(user["user"]));
					dpp::get_user_cache()->store(u);
				} else {
					u->refcount++;
				}
				dpp::guild_member* gm = new dpp::guild_member();
				gm->fill_from_json(&(user["user"]), g, u);
				g->members->insert(std::make_pair(u->id, gm));
			}
		}
		/* Store emojis */
		g->emojis.reserve(d["emojis"].size());
		g->emojis.clear();
		for (auto & emoji : d["emojis"]) {
			dpp::emoji* e = dpp::find_emoji(SnowflakeNotNull(&emoji, "id"));
			if (!e) {
				e = new dpp::emoji();
				e->fill_from_json(&emoji);
				dpp::get_emoji_cache()->store(e);
			}
			g->emojis.push_back(e->id);
		}
	}
	dpp::get_guild_cache()->store(g);
	if (newguild && g->id && (client->intents & dpp::i_guild_members)) {
		json chunk_req = json({{"op", 8}, {"d", {{"guild_id",std::to_string(g->id)},{"query",""},{"limit",0}}}});
		if (client->intents & dpp::i_guild_presences) {
			chunk_req["d"]["presences"] = true;
		}
		client->QueueMessage(chunk_req.dump());
	}

	if (client->creator->dispatch.guild_create) {
		dpp::guild_create_t gc(client, raw);
		gc.created = g;
		client->creator->dispatch.guild_create(gc);
	}
}

}};