#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <spdlog/spdlog.h>

uint64_t SnowflakeNotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() ? from_string<uint64_t>((*j)[keyname].get<std::string>(), std::dec) : 0;
}

std::string StringNotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() ? (*j)[keyname].get<std::string>() : "";
}

uint32_t Int32NotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() ? (*j)[keyname].get<uint32_t>() : 0;
}

uint16_t Int16NotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() ? (*j)[keyname].get<uint16_t>() : 0;
}

uint8_t Int8NotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() ? (*j)[keyname].get<uint8_t>() : 0;
}

bool BoolNotNull(json* j, const char *keyname)
{
	return (j->find(keyname) != j->end() && !(*j)[keyname].is_null() && (*j)[keyname].get<bool>() == true);
}

std::map<std::string, std::function<void(DiscordClient* client, json &j)>> events = {
	{
		"READY",
		[](DiscordClient* client, json &j) {
			client->logger->info("Shard {}/{} ready!", client->shard_id, client->max_shards);
			client->sessionid = j["d"]["session_id"];
		}
	},
	{
		"RESUMED",
		[](DiscordClient* client, json &j) {
			client->logger->debug("Successfully resumed session id {}", client->sessionid);
		}
	},
	{
		"GUILD_CREATE",
		[](DiscordClient* client, json &j) {
			dpp::guild* g = new dpp::guild();
			json& d = j["d"];
			g->fill_from_json(&d);
			if (!g->is_unavailable()) {
				/* Store guild roles */
				for (auto & role : d["roles"]) {
					dpp::role *r = new dpp::role();
					r->fill_from_json(&role);
					dpp::store_role(r);
					g->roles.push_back(r->id);
				}

				/* Store guild channels */
				for (auto & channel : d["channels"]) {
					dpp::channel *c = new dpp::channel();
					c->fill_from_json(&channel);
					dpp::store_channel(c);
					g->channels.push_back(c->id);
				}

				/* Store guild members */
				for (auto & user : d["members"]) {
					dpp::user *u = new dpp::user();
					u->fill_from_json(&(user["user"]));
					dpp::guild_member* gm = new dpp::guild_member();
					gm->fill_from_json(&user, g, u);

					g->members[u->id] = gm;
					dpp::store_user(u);
				}
			}
			dpp::store_guild(g);
		}
	},
	{
		"MESSAGE_UPDATE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("MESSAGE_UPDATE");
		}
	},
	{
		"MESSAGE_DELETE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("MESSAGE_DELETE");
		}
	},
	{
		"MESSAGE_CREATE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("MESSAGE_CREATE");
		}
	},
	{
		"CHANNEL_UPDATE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("CHANNEL_UPDATE");
		}
	},
	{
		"GUILD_ROLE_UPDATE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("GUILD_ROLE_UPDATE");
		}
	},
	{
		"MESSAGE_DELETE_BULK",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("MESSAGE_DELETE_BULK");
		}
	},
	{
		"CHANNEL_CREATE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("CHANNEL_CREATE");
		}
	},
	{
		"CHANNEL_DELETE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("CHANNEL_DELETE");
		}
	},

};

void DiscordClient::HandleEvent(const std::string &event, json &j)
{
	auto ev_iter = events.find(event);
	if (ev_iter != events.end()) {
		ev_iter->second(this, j);
	} else {
		logger->debug("Unhamdled event: {}", event);
	}
}
