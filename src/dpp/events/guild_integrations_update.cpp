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

void guild_integrations_update::handle(class DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.guild_integrations_update) {
		json& d = j["d"];
		dpp::guild_integrations_update_t giu(raw);
		giu.updating_guild = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
		client->creator->dispatch.guild_integrations_update(giu);
	}
}

