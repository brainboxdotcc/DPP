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
void channel_pins_update::handle(DiscordClient* client, json &j, const std::string &raw) {

	if (client->creator->dispatch.channel_pins_update) {
		json& d = j["d"];
		dpp::channel_pins_update_t cpu(client, raw);
		cpu.pin_channel = dpp::find_channel(SnowflakeNotNull(&d, "channel_id"));
		cpu.pin_guild = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
		cpu.timestamp = TimestampNotNull(&d, "last_pin_timestamp");

		client->creator->dispatch.channel_pins_update(cpu);

	}

}

}};