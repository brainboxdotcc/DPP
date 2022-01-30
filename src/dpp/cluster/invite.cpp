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

namespace dpp {

void cluster::guild_get_invites(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "invites", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		invite_map invites;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_invite : j) {
				invites[string_not_null(&curr_invite, "code")] = invite().fill_from_json(&curr_invite);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("invite_map", invites, http));
		}
	});
}


void cluster::invite_delete(const std::string &invitecode, command_completion_event_t callback) {
	this->post_rest(API_PATH "/invites", dpp::url_encode(invitecode), "", m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("invite", invite().fill_from_json(&j), http));
		}
	});
}


void cluster::invite_get(const std::string &invitecode, command_completion_event_t callback) {
	this->post_rest(API_PATH "/invites", dpp::url_encode(invitecode) + "?with_counts=true&with_expiration=true", "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("invite", invite().fill_from_json(&j), http));
		}
	});
}

};