#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <spdlog/spdlog.h>

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
			client->logger->debug("GUILD_CREATE");
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
