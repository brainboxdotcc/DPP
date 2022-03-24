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
#include <dpp/appcommand.h>
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
	/* We must set here because we cant pass it through the nlohmann from_json() */
	i.cache_policy = client->creator->cache_policy;
	i.fill_from_json(&d);
	/* There are two types of interactions, component interactions and
	 * slash command interactions. Both fire different library events
	 * so ensure they are dispatched properly.
	 */
	if (i.type == it_application_command) {
		if (!client->creator->on_interaction_create.empty()) {
			dpp::interaction_create_t ic(client, raw);
			ic.command = i;
			client->creator->on_interaction_create.call(ic);
		}
	} else if (i.type == it_modal_submit) {
		if (!client->creator->on_form_submit.empty()) {
			dpp::form_submit_t fs(client, raw);
			fs.custom_id = string_not_null(&(d["data"]), "custom_id");
			fs.command = i;
			for (auto & c : d["data"]["components"]) {
				fs.components.push_back(dpp::component().fill_from_json(&c));
			}
			client->creator->on_form_submit.call(fs);
		}
	} else if (i.type == it_autocomplete) {
		// "data":{"id":"903319628816728104","name":"blep","options":[{"focused":true,"name":"animal","type":3,"value":"a"}],"type":1}
		if (!client->creator->on_autocomplete.empty()) {
			dpp::autocomplete_t ac(client, raw);
			ac.id = snowflake_not_null(&(d["data"]), "id");
			ac.name = string_not_null(&(d["data"]), "name");
			for (auto & o : d["data"]["options"]) {
				dpp::command_option opt;
				opt.name = string_not_null(&o, "name");
				opt.type = (dpp::command_option_type)int8_not_null(&o, "type");
				if (o.contains("value") && !o.at("value").is_null()) {
					switch (opt.type) {
						case co_boolean:
							opt.value = o.at("value").get<bool>();
							break;
						case co_channel:
						case co_role:
						case co_user:
						case co_attachment:
						case co_mentionable:
							opt.value = snowflake_not_null(&o, "value");
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
				opt.focused = bool_not_null(&o, "focused");
				ac.options.emplace_back(opt);
			}
			ac.command = i;
			client->creator->on_autocomplete.call(ac);
		}
	} else if (i.type == it_component_button) {
		dpp::component_interaction bi = std::get<component_interaction>(i.data);
		if (bi.component_type == cotype_button) {
			if (!client->creator->on_button_click.empty()) {
				dpp::button_click_t ic(client, raw);
				ic.command = i;
				ic.custom_id = bi.custom_id;
				ic.component_type = bi.component_type;
				client->creator->on_button_click.call(ic);
			}
		}
		if (bi.component_type == cotype_select) {
			if (!client->creator->on_select_click.empty()) {
				dpp::select_click_t ic(client, raw);
				ic.command = i;
				ic.custom_id = bi.custom_id;
				ic.component_type = bi.component_type;
				ic.values = bi.values;
				client->creator->on_select_click.call(ic);
			}
		}
	}
}

}};