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

using json = nlohmann::json;

void message_create::handle(class DiscordClient* client, json &j) {
	dpp::message_create_t msg;
	dpp::message m;
	msg.msg = &m;
	client->creator->dispatch.message_create(msg);
}

