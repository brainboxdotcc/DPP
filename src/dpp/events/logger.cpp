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

void logger::handle(class DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.log) {
		dpp::log_t logmsg(raw);
		logmsg.severity = (dpp::loglevel)from_string<uint32_t>(raw.substr(0, raw.find(';')), std::dec);
		logmsg.message = raw.substr(raw.find(';') + 1, raw.length());
		client->creator->dispatch.log(logmsg);
	}
}

