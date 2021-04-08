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

void voice_server_update::handle(class DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.voice_server_update) {
		json &d = j["d"];
		dpp::voice_server_update_t vsu(raw);
		vsu.guild_id = SnowflakeNotNull(&d, "guild_id");
		vsu.token = StringNotNull(&d, "token");
		vsu.endpoint = StringNotNull(&d, "endpoint");
		client->creator->dispatch.voice_server_update(vsu);
	}
}

