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
void voice_state_update::handle(DiscordClient* client, json &j, const std::string &raw) {

	json& d = j["d"];
	dpp::voice_state_update_t vsu(client, raw);
	vsu.state = dpp::voicestate().fill_from_json(&d);
	if (vsu.state.user_id == client->creator->me.id)
	{
		std::lock_guard<std::mutex> lock(client->voice_mutex);
		auto v = client->connecting_voice_channels.find(vsu.state.guild_id);
		/* Check to see if we have a connection to a voice channel in progress on this guild */
		if (v != client->connecting_voice_channels.end()) {
			v->second->session_id = vsu.state.session_id;
			if (v->second->is_ready() && !v->second->is_active()) {
				v->second->connect(vsu.state.guild_id);
			}
		}
	}

	if (client->creator->dispatch.voice_state_update) {
		client->creator->dispatch.voice_state_update(vsu);
	}
}

}};