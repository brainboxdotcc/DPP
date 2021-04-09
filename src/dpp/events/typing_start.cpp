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
void typing_start::handle(class DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.typing_start) {
		json& d = j["d"];
		dpp::typing_start_t ts(raw);
		ts.typing_guild = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
		ts.typing_channel = dpp::find_channel(SnowflakeNotNull(&d, "channel_id"));
		ts.typing_user = dpp::find_user(SnowflakeNotNull(&d, "user_id"));
		ts.timestamp = TimestampNotNull(&d, "timestamp");
		client->creator->dispatch.typing_start(ts);
	}
}

