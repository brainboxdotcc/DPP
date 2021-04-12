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
void user_update::handle(DiscordClient* client, json &j, const std::string &raw) {
	json& d = j["d"];

	dpp::snowflake user_id = SnowflakeNotNull(&d, "id");
	if (user_id) {
		dpp::user* u = dpp::find_user(user_id);
		u->fill_from_json(&d);

		if (client->creator->dispatch.user_update) {
			dpp::user_update_t uu(raw);
			uu.updated = u;
			client->creator->dispatch.user_update(uu);
		}

	}
}

}};