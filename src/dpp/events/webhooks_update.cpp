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
void webhooks_update::handle(DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.webhooks_update) {
		json& d = j["d"];
		dpp::webhooks_update_t wu(client, raw);
		wu.webhook_guild = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
		wu.webhook_channel = dpp::find_channel(SnowflakeNotNull(&d, "channel_id"));
		client->creator->dispatch.webhooks_update(wu);
	}
}

}};