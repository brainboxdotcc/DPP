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

void voice_state_update::handle(class DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.voice_state_update) {
		json& d = j["d"];
		dpp::voice_state_update_t vsu(raw);
		vsu.state = dpp::voicestate().fill_from_json(&d);
		client->creator->dispatch.voice_state_update(vsu);
	}
}

