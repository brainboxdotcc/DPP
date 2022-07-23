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
#include <dpp/channel.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp { namespace events {

using namespace dpp;
void thread_delete::handle(discord_client* client, json& j, const std::string& raw) {
	json& d = j["d"];

	dpp::thread t;
	t.fill_from_json(&d);
	dpp::guild* g = dpp::find_guild(t.guild_id);
	if (g) {
		auto gt = std::find(g->threads.begin(), g->threads.end(), t.id);
		if (gt != g->threads.end()) {
			g->threads.erase(gt);
		}
		if (!client->creator->on_thread_delete.empty()) {
			dpp::thread_delete_t td(client, raw);
			td.deleted = t;
			td.deleting_guild = g;
			client->creator->on_thread_delete.call(td);
		}
	}
}
}};
