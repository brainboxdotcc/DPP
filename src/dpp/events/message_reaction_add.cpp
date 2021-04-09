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

/**
 * @brief Handle event
 * 
 * @param client Websocket client (current shard)
 * @param j JSON data for the event
 * @param raw Raw JSON string
 */
void message_reaction_add::handle(class DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.message_reaction_add) {
		json &d = j["d"];
		dpp::message_reaction_add_t mra(raw);
		mra.reacting_guild = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
		mra.reacting_user = dpp::find_user(SnowflakeNotNull(&d, "user_id"));
		mra.reacting_channel = dpp::find_channel(SnowflakeNotNull(&d, "channel_id"));
		mra.message_id = SnowflakeNotNull(&d, "message_id");
		mra.reacting_emoji = dpp::find_emoji(SnowflakeNotNull(&(d["emoji"]), "id"));
		if (mra.reacting_user && mra.reacting_channel && mra.message_id) {
			client->creator->dispatch.message_reaction_add(mra);
		}
	}
}

