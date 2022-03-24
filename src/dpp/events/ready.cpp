/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2021 Craig Edwards and D++ contributors 
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/
#include <dpp/discordevents.h>
#include <dpp/cluster.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt-minimal.h>

using json = nlohmann::json;

namespace dpp { namespace events {

using namespace dpp;

#ifndef _DOXYGEN_
std::mutex protect_the_loot;
#endif

/**
 * @brief Handle event
 * 
 * @param client Websocket client (current shard)
 * @param j JSON data for the event
 * @param raw Raw JSON string
 */
void ready::handle(discord_client* client, json &j, const std::string &raw) {
	client->log(dpp::ll_info, fmt::format("Shard id {} ({}/{}) ready!", client->shard_id, client->shard_id + 1, client->max_shards));
	client->sessionid = j["d"]["session_id"];

	client->ready = true;

	/* Mutex this to make sure multiple threads don't change it at the same time */
	{
		std::lock_guard<std::mutex> lockit(protect_the_loot);
		client->creator->me.fill_from_json(&(j["d"]["user"]));
	}

	if (!client->creator->on_ready.empty()) {
		dpp::ready_t r(client, raw);
		r.session_id = client->sessionid;
		r.shard_id = client->shard_id;
		client->creator->on_ready.call(r);
	}
}

}};