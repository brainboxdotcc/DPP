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

void guild_member_remove::handle(class DiscordClient* client, json &j) {
	json d = j["d"];

	dpp::guild_member_remove_t gmr;

	gmr.removing_guild = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
	gmr.removed = dpp::find_user(SnowflakeNotNull(&(d["user"]), "id"));

	if (client->creator->dispatch.guild_member_remove)
		client->creator->dispatch.guild_member_remove(gmr);

	if (gmr.removing_guild) {
		auto i = gmr.removing_guild->members.find(gmr.removed->id);
		if (i != gmr.removing_guild->members.end()) {
			delete i->second;
			gmr.removing_guild->members.erase(i);
		}
	}
}

