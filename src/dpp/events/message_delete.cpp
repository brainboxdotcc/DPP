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

void message_delete::handle(class DiscordClient* client, json &j) {
	json d = j["d"];
	dpp::message_delete_t msg;
	dpp::message m;
	m.fill_from_json(&d);
	msg.deleted = &m;

	if (client->creator->dispatch.message_delete)
		client->creator->dispatch.message_delete(msg);

}

