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

/**
 * @brief Handle event
 * 
 * @param client Websocket client (current shard)
 * @param j JSON data for the event
 * @param raw Raw JSON string
 */
void guild_emojis_update::handle(class DiscordClient* client, json &j, const std::string &raw) {
	json& d = j["d"];
	dpp::guild* g = dpp::find_guild(SnowflakeNotNull(&d, "guild_id"));
	if (g) {
		g->emojis.clear();
		for (auto & emoji : d["emojis"]) {
			dpp::emoji* e = dpp::find_emoji(SnowflakeNotNull(&emoji, "id"));
			if (!e) {
				e = new dpp::emoji();
				e->fill_from_json(&emoji);
				dpp::get_emoji_cache()->store(e);
			}
			g->emojis.push_back(e->id);
		}
		if (client->creator->dispatch.guild_emojis_update) {
			dpp::guild_emojis_update_t geu(raw);
			geu.emojis = g->emojis;
			geu.updating_guild = g;
			client->creator->dispatch.guild_emojis_update(geu);
		}
	}
}

