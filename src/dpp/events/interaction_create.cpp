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
void interaction_create::handle(discord_client* client, json &j, const std::string &raw) {
	json& d = j["d"];
	dpp::interaction i;
	i.fill_from_json(&d);
	/* There are two types of interactions, component interactions and
	 * slash command interactions. Both fire different library events
	 * so ensure they are dispatched properly.
	 */
	if (i.type == it_application_command) {
		if (!client->creator->dispatch.interaction_create.empty()) {
			dpp::interaction_create_t ic(client, raw);
			ic.command = i;
			call_event(client->creator->dispatch.interaction_create, ic);
		}
	} else if (i.type == it_autocomplete) {
		// "data":{"id":"903319628816728104","name":"blep","options":[{"focused":true,"name":"animal","type":3,"value":"a"}],"type":1}
		if (!client->creator->dispatch.autocomplete.empty()) {
			dpp::autocomplete_t ac(client, raw);
			ac.id = SnowflakeNotNull(&(d["data"]), "id");
			ac.name = StringNotNull(&(d["data"]), "name");
			for (auto & o : d["data"]["options"]) {
				dpp::command_option opt;
				opt.name = StringNotNull(&o, "name");
				opt.type = (dpp::command_option_type)Int8NotNull(&o, "type");
				if (o.contains("value") && !o.at("value").is_null()) {
					switch (opt.type) {
						case co_boolean:
							opt.value = o.at("value").get<bool>();
							break;
						case co_channel:
						case co_role:
						case co_user:
						case co_mentionable:
							opt.value = SnowflakeNotNull(&o, "value");
							break;
						case co_integer:
							opt.value = o.at("value").get<int64_t>();
							break;
						case co_string:
							opt.value = o.at("value").get<std::string>();
							break;
						case co_number:
							opt.value = o.at("value").get<double>();
							break;
						case co_sub_command:
						case co_sub_command_group:
							/* Silences warning on clang, handled elsewhere */
						break;
					}
				}
				opt.focused = BoolNotNull(&o, "focused");
				ac.options.emplace_back(opt);
			}
			ac.command = i;
			call_event(client->creator->dispatch.autocomplete, ac);
		}
	} else if (i.type == it_component_button) {
		dpp::component_interaction bi = std::get<component_interaction>(i.data);
		if (bi.component_type == cotype_button) {
			if (!client->creator->dispatch.button_click.empty()) {
				dpp::button_click_t ic(client, raw);
				ic.command = i;
				ic.custom_id = bi.custom_id;
				ic.component_type = bi.component_type;
				call_event(client->creator->dispatch.button_click, ic);
			}
		}
		if (bi.component_type == cotype_select) {
			if (!client->creator->dispatch.select_click.empty()) {
				dpp::select_click_t ic(client, raw);
				ic.command = i;
				ic.custom_id = bi.custom_id;
				ic.component_type = bi.component_type;
				ic.values = bi.values;
				call_event(client->creator->dispatch.select_click, ic);
			}
		}
	}
}

}};