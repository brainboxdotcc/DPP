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

void integration_update::handle(class DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.integration_update) {
		json& d = j["d"];
		dpp::integration_update_t iu(raw);
		iu.updated_integration = dpp::integration().fill_from_json(&d);
		client->creator->dispatch.integration_update(iu);
	}
}

