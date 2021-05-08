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
#include <dpp/discord.h>
#include <dpp/event.h>
#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp { namespace events {

using namespace dpp;

/**
 * @brief Handle event
 * 
 * @param client Websocket client (current shard)
 * @param j JSON data for the event
 * @param raw Raw JSON string
 */
void interaction_create::handle(DiscordClient* client, json &j, const std::string &raw) {
	json& d = j["d"];
	dpp::interaction i;
	i.fill_from_json(&d);
	/* There are two types of interactions, component interactions and
	 * slash command interactions. Both fire different library events
	 * so ensure they are dispatched properly.
	 */
	if (i.type == it_application_command) {
		if (client->creator->dispatch.interaction_create) {
			dpp::interaction_create_t ic(client, raw);
			ic.command = i;
			client->creator->dispatch.interaction_create(ic);
		}
	} else if (i.type == it_component_button) {
		if (client->creator->dispatch.button_click) {
			dpp::button_click_t ic(client, raw);
			dpp::button_interaction bi = std::get<button_interaction>(i.data);
			ic.command = i;
			ic.custom_id = bi.custom_id;
			ic.component_type = bi.component_type;
			client->creator->dispatch.button_click(ic);
		}
	}
}

}};