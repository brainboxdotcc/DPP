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
#include <fmt/format.h>

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
void channel_create::handle(DiscordClient* client, json &j, const std::string &raw) {
	json& d = j["d"];
	
	dpp::channel* c = dpp::find_channel(SnowflakeNotNull(&d, "id"));
	if (!c) {
		c = new dpp::channel();
	}
	c->fill_from_json(&d);
	dpp::get_channel_cache()->store(c);
	if (c->recipients.size()) {
		for (auto & u : c->recipients) {
			client->log(dpp::ll_debug, fmt::format("Got a DM channel {} for user {}", c->id, u));
			client->creator->set_dm_channel(u, c->id);
		}
	}
	dpp::guild* g = dpp::find_guild(c->guild_id);
	if (g) {
		g->channels.push_back(c->id);

		if (client->creator->dispatch.channel_create) {
			dpp::channel_create_t cc(client, raw);
			cc.created = c;
			cc.creating_guild = g;
			client->creator->dispatch.channel_create(cc);
		}
	}
}

}};