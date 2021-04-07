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
#include <dpp/dispatcher.h>
#include <dpp/discordevents.h>

using json = nlohmann::json;

void message_create::handle(class DiscordClient* client, json &j) {

	if (client->creator->dispatch.message_create) {
		json d = j["d"];
		dpp::message_create_t msg(d.dump());
		dpp::message m;
		m.fill_from_json(&d);	
		msg.msg = &m;
		client->creator->dispatch.message_create(msg);
	}
}

