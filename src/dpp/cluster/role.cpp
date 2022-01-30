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
#include <dpp/role.h>
#include <dpp/cluster.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

void cluster::role_create(const class role &r, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(r.guild_id), "roles", m_post, r.build_json(), [r, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("role", role().fill_from_json(r.guild_id, &j), http));
		}
	});
}



void cluster::role_delete(snowflake guild_id, snowflake role_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "roles/" + std::to_string(role_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::role_edit(const class role &r, command_completion_event_t callback) {
	json j = r.build_json(true);
	auto p = j.find("position");
	if (p != j.end()) {
		j.erase(p);
	}
	this->post_rest(API_PATH "/guilds", std::to_string(r.guild_id), "roles/" + std::to_string(r.id) , m_patch, j.dump(), [r, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("role", role().fill_from_json(r.guild_id, &j), http));
		}
	});
}


void cluster::roles_edit_position(snowflake guild_id, const std::vector<role> &roles, command_completion_event_t callback) {
	if (roles.empty()) {
		return;
	}
	json j = json::array();
	for (auto & r : roles) {
		j.push_back({ {"id", r.id}, {"position", r.position} });
	}
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "roles", m_patch, j.dump(), [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			role_map roles;
			confirmation_callback_t e("confirmation", confirmation(), http);
			if (!e.is_error()) {
				for (auto & curr_role : j) {
					roles[snowflake_not_null(&curr_role, "id")] = role().fill_from_json(guild_id, &curr_role);
				}
			}
			callback(confirmation_callback_t("role_map", roles, http));
		}
	});
}


void cluster::roles_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "roles", m_get, "", [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			role_map roles;
			confirmation_callback_t e("confirmation", confirmation(), http);
			if (!e.is_error()) {
				for (auto & curr_role : j) {
					roles[snowflake_not_null(&curr_role, "id")] = role().fill_from_json(guild_id, &curr_role);
				}
			}
			callback(confirmation_callback_t("role_map", roles, http));
		}
	});
}

};