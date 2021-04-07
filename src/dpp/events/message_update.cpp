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

void message_update::handle(class DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.message_update) {
		json d = j["d"];
		dpp::message_update_t msg(raw);
		dpp::message m;
		m.fill_from_json(&d);
	      	msg.updated = &m;
		client->creator->dispatch.message_update(msg);
	}

}

