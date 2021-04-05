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

void guild_create::handle(class DiscordClient* client, json &j) {
	json& d = j["d"];
	dpp::guild* g = dpp::find_guild(SnowflakeNotNull(&d, "id"));
	if (!g) {
		g = new dpp::guild();
	}
	g->fill_from_json(&d);
	if (!g->is_unavailable()) {
		/* Store guild roles */
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
		for (auto & channel : d["channels"]) {
			dpp::channel *c = new dpp::channel();
			c->fill_from_json(&channel);
			dpp::get_channel_cache()->store(c);
			g->channels.push_back(c->id);
		}

		/* Store guild members */
		for (auto & user : d["members"]) {
			dpp::user* u = dpp::find_user(SnowflakeNotNull(&(user["user"]), "id"));
			if (!u) {
				u = new dpp::user();
				u->fill_from_json(&(user["user"]));
				dpp::get_user_cache()->store(u);
			}
			dpp::guild_member* gm = new dpp::guild_member();
			gm->fill_from_json(&(user["user"]), g, u);

			g->members[u->id] = gm;
		}

		/* Store emojis */
		for (auto & emoji : d["emojis"]) {
			dpp::emoji* e = dpp::find_emoji(SnowflakeNotNull(&emoji, "id"));
			if (!e) {
				e = new dpp::emoji();
				e->fill_from_json(&emoji);
				dpp::get_emoji_cache()->store(e);
			}
			g->emojis[e->id] = e;
		}

	}
	dpp::get_guild_cache()->store(g);
	if (client->intents & dpp::GUILD_MEMBERS) {
		client->add_chunk_queue(g->id);
	}

	dpp::guild_create_t gc;
	gc.created = g;
	if (client->creator->dispatch.guild_create)
		client->creator->dispatch.guild_create(gc);
}

