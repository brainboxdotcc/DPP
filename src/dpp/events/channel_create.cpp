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

void channel_create::handle(class DiscordClient* client, json &j, const std::string &raw) {
	json& d = j["d"];
	dpp::channel* c = dpp::find_channel(SnowflakeNotNull(&d, "id"));
	if (!c) {
		c = new dpp::channel();
	}
	c->fill_from_json(&d);
	dpp::get_channel_cache()->store(c);
	dpp::guild* g = dpp::find_guild(c->guild_id);
	if (g) {
		g->channels.push_back(c->id);

		if (client->creator->dispatch.channel_create) {
			dpp::channel_create_t cc(raw);
			cc.created = c;
			cc.creating_guild = g;
			client->creator->dispatch.channel_create(cc);
		}
	}
}

