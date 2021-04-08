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

void invite_delete::handle(class DiscordClient* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.invite_delete) {
		json& d = j["d"];
		dpp::invite_delete_t cd(raw);
		cd.deleted_invite = dpp::invite().fill_from_json(&d);
		client->creator->dispatch.invite_delete(cd);
	}
}

