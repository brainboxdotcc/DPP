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
void voice_server_update::handle(DiscordClient* client, json &j, const std::string &raw) {

	json &d = j["d"];
	dpp::voice_server_update_t vsu(raw);
	vsu.guild_id = SnowflakeNotNull(&d, "guild_id");
	vsu.token = StringNotNull(&d, "token");
	vsu.endpoint = StringNotNull(&d, "endpoint");

	{
		std::lock_guard<std::mutex> lock(client->voice_mutex);
		auto v = client->connecting_voice_channels.find(vsu.guild_id);
		/* Check to see if there is a connection in progress for a voice channel on this guild */
		if (v != client->connecting_voice_channels.end()) {
			if (!v->second->is_ready()) {
				v->second->token = vsu.token;
				v->second->websocket_hostname = vsu.endpoint;
				if (!v->second->is_active()) {
					v->second->connect(vsu.guild_id);
				}
			}
		}
	}

	if (client->creator->dispatch.voice_server_update) {
		client->creator->dispatch.voice_server_update(vsu);
	}
}

}};