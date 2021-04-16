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
void presence_update::handle(DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.presence_update) {
		json& d = j["d"];
		dpp::presence_update_t pu(client, raw);
		pu.rich_presence = dpp::presence().fill_from_json(&d);
		client->creator->dispatch.presence_update(pu);
	}
}

}};