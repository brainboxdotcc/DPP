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

void message_reaction_remove_emoji::handle(class DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.message_reaction_remove_emoji) {
		json &d = j["d"];
		dpp::message_reaction_remove_emoji_t mrre(raw);
		mrre.reacting_guild = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
		mrre.reacting_channel = dpp::find_channel(SnowflakeNotNull(&d, "channel_id"));
		mrre.message_id = SnowflakeNotNull(&d, "message_id");
		mrre.reacting_emoji = dpp::find_emoji(SnowflakeNotNull(&(d["emoji"]), "id"));
		if (mrre.reacting_channel && mrre.message_id) {
			client->creator->dispatch.message_reaction_remove_emoji(mrre);
		}
	}

}

