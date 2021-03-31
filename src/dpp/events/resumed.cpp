#include <dpp/discord.h>
#include <dpp/event.h>
#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void resumed::handle(class DiscordClient* client, json &j) {
	client->logger->debug("Successfully resumed session id {}", client->sessionid);
	dpp::resumed_t r;
	r.session_id = client->sessionid;
	r.shard_id = client->shard_id;
	if (client->creator->dispatch.resumed)
		client->creator->dispatch.resumed(r);
}

