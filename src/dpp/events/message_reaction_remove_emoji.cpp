#include <dpp/discord.h>
#include <dpp/event.h>
#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <nlohmann/json.hpp>
#include <dpp/discordevents.h>

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
void message_reaction_remove_emoji::handle(DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.message_reaction_remove_emoji) {
		json &d = j["d"];
		dpp::message_reaction_remove_emoji_t mrre(client, raw);
		mrre.reacting_guild = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
		mrre.reacting_channel = dpp::find_channel(SnowflakeNotNull(&d, "channel_id"));
		mrre.message_id = SnowflakeNotNull(&d, "message_id");
		mrre.reacting_emoji = dpp::find_emoji(SnowflakeNotNull(&(d["emoji"]), "id"));
		if (mrre.reacting_channel && mrre.message_id) {
			client->creator->dispatch.message_reaction_remove_emoji(mrre);
		}
	}

}

}};