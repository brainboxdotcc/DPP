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
void message_delete_bulk::handle(DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.message_delete_bulk) {
		json& d = j["d"];
		dpp::message_delete_bulk_t msg(client, raw);
		msg.deleting_guild = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
		msg.deleting_channel = dpp::find_channel(SnowflakeNotNull(&d, "channel_id"));
		msg.deleting_user = dpp::find_user(SnowflakeNotNull(&d, "user_id"));
		for (auto& m : d["ids"]) {
			msg.deleted.push_back(from_string<uint64_t>(m.get<std::string>(), std::dec));
		}
		client->creator->dispatch.message_delete_bulk(msg);
	}

}

}};