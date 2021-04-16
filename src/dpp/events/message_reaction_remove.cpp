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
void message_reaction_remove::handle(DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.message_reaction_remove) {
		json &d = j["d"];
		dpp::message_reaction_remove_t mrr(client, raw);
		mrr.reacting_guild = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
		mrr.reacting_user = dpp::find_user(SnowflakeNotNull(&d, "user_id"));
		mrr.reacting_channel = dpp::find_channel(SnowflakeNotNull(&d, "channel_id"));
		mrr.message_id = SnowflakeNotNull(&d, "message_id");
		mrr.reacting_emoji = dpp::find_emoji(SnowflakeNotNull(&(d["emoji"]), "id"));
		if (mrr.reacting_user && mrr.reacting_channel && mrr.message_id) {
			client->creator->dispatch.message_reaction_remove(mrr);
		}
	}
}

}};