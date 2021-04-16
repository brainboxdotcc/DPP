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
void integration_delete::handle(DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.integration_delete) {
		json& d = j["d"];
		dpp::integration_delete_t id(client, raw);
		id.deleted_integration = dpp::integration().fill_from_json(&d);
		client->creator->dispatch.integration_delete(id);
	}
}

}};