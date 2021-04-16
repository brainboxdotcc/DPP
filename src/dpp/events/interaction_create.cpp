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
void interaction_create::handle(DiscordClient* client, json &j, const std::string &raw) {
	json& d = j["d"];
	dpp::interaction i;
	i.fill_from_json(&d);
	if (client->creator->dispatch.interaction_create) {
		dpp::interaction_create_t ic(raw);
		ic.command = i;
		client->creator->dispatch.interaction_create(ic);
	}
}

}};