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
#include <dpp/cluster.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt-minimal.h>

namespace dpp {

void cluster::guild_add_member(const guild_member& gm, const std::string &access_token, command_completion_event_t callback) {
	json j = json::parse(gm.build_json());
	j["access_token"] = access_token;
	this->post_rest(API_PATH "/guilds", std::to_string(gm.guild_id), "members/" + std::to_string(gm.user_id), m_put, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::guild_edit_member(const guild_member& gm, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(gm.guild_id), "members/" + std::to_string(gm.user_id), m_patch, gm.build_json(), [&gm, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild_member", guild_member().fill_from_json(&j, gm.guild_id, gm.user_id), http));
		}
	});
}


void cluster::guild_get_member(snowflake guild_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members/" + std::to_string(user_id), m_get, "", [callback, guild_id, user_id](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild_member", guild_member().fill_from_json(&j, guild_id, user_id), http));
		}
	});
}


void cluster::guild_get_members(snowflake guild_id, uint16_t limit, snowflake after, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), fmt::format("members?limit={}&after={}", limit, after), m_get, "", [callback, guild_id](json &j, const http_request_completion_t& http) {
		guild_member_map guild_members;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_member : j) {
				if (curr_member.find("user") != curr_member.end()) {
					snowflake user_id = snowflake_not_null(&(curr_member["user"]), "id");
					guild_members[user_id] = guild_member().fill_from_json(&curr_member, guild_id, user_id);
				}
			}
		}
		if (callback) {
			callback(confirmation_callback_t("guild_member_map", guild_members, http));
		}
	});
}


void cluster::guild_member_add_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members/" + std::to_string(user_id) + "/roles/" + std::to_string(role_id), m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::guild_member_delete(snowflake guild_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members/" + std::to_string(user_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::guild_member_delete_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members/" + std::to_string(user_id) + "/roles/" + std::to_string(role_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::guild_member_move(const snowflake channel_id, const snowflake guild_id, const snowflake user_id, command_completion_event_t callback) {
    json j;
    j["channel_id"] = channel_id;

    this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members/" + std::to_string(user_id), m_patch, j.dump(), [guild_id, user_id, callback](json &j, const http_request_completion_t& http) {
	if (callback) {
	    callback(confirmation_callback_t("guild_member", guild_member().fill_from_json(&j, guild_id, user_id), http));
	}
    });
}


void cluster::guild_search_members(snowflake guild_id, const std::string& query, uint16_t limit, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), fmt::format("members/search?query=\"{}\"&limit={}", dpp::url_encode(query), limit), m_get, "", [callback, guild_id] (json &j, const http_request_completion_t& http) {
		guild_member_map guild_members;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_member : j) {
				if (curr_member.find("user") != curr_member.end()) {
					snowflake user_id = snowflake_not_null(&(curr_member["user"]), "id");
					guild_members[user_id] = guild_member().fill_from_json(&curr_member, guild_id, user_id);
				}
			}
		}
		if (callback) {
			callback(confirmation_callback_t("guild_member_map", guild_members, http));
		}
	});
}

};