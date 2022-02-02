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
#include <dpp/dtemplate.h>
#include <dpp/cluster.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

void cluster::guild_create_from_template(const std::string &code, const std::string &name, command_completion_event_t callback) {
	json params;
	params["name"] = name;
	this->post_rest(API_PATH "/guilds", "templates", code, m_post, params.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(nullptr, &j), http));
		}
	});
}


void cluster::guild_template_create(snowflake guild_id, const std::string &name, const std::string &description, command_completion_event_t callback) {
	json params;
	params["name"] = name;
	params["description"] = description;
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "templates", m_post, params.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}


void cluster::guild_template_delete(snowflake guild_id, const std::string &code, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "templates/" + code, m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}


void cluster::guild_template_modify(snowflake guild_id, const std::string &code, const std::string &name, const std::string &description, command_completion_event_t callback) {
	json params;
	params["name"] = name;
	params["description"] = description;
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "templates/" + code, m_patch, params.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}


void cluster::guild_templates_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "templates", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		dtemplate_map dtemplates;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_dtemplate : j) {
				dtemplates[snowflake_not_null(&curr_dtemplate, "id")] = dtemplate().fill_from_json(&curr_dtemplate);
			}
		}
		if (callback) {
				callback(confirmation_callback_t("dtemplate_map", dtemplates, http));
		}
	});
}


void cluster::guild_template_sync(snowflake guild_id, const std::string &code, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "templates/" + code, m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}


void cluster::template_get(const std::string &code, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", "templates", code, m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}

};