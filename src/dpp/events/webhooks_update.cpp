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

void webhooks_update::handle(class DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.webhooks_update) {
		json& d = j["d"];
		dpp::webhooks_update_t wu(raw);
		wu.webhook_guild = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
		wu.webhook_channel = dpp::find_channel(SnowflakeNotNull(&d, "channel_id"));
		client->creator->dispatch.webhooks_update(wu);
	}
}

