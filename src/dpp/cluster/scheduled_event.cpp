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
#include <dpp/cluster.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

void cluster::guild_events_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "/scheduled-events?with_user_count=true", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			scheduled_event_map events;
			confirmation_callback_t e("confirmation", confirmation(), http);
			if (!e.is_error()) {
				for (auto & curr_event : j) {
					events[SnowflakeNotNull(&curr_event, "id")] = scheduled_event().fill_from_json(&curr_event);
				}
			}
			callback(confirmation_callback_t("scheduled_event_map", events, http));
		}
	});
}

void cluster::guild_event_users_get(snowflake guild_id, snowflake event_id, command_completion_event_t callback, uint8_t limit) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "/scheduled-events/" + std::to_string(event_id) + "/users?limit=" + std::to_string(limit), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			user_map users;
			confirmation_callback_t e("confirmation", confirmation(), http);
			if (!e.is_error()) {
				for (auto & curr_user : j) {
					users[SnowflakeNotNull(&curr_user, "id")] = user().fill_from_json(&curr_user);
				}
			}
			callback(confirmation_callback_t("user_map", users, http));
		}
	});
}

void cluster::guild_event_create(const scheduled_event& event, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(event.guild_id), "/scheduled-events", m_post, event.build_json(false), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("scheduled_event", scheduled_event().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_event_delete(snowflake event_id, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "/scheduled-events/" + std::to_string(event_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_event_edit(const scheduled_event& event, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(event.guild_id), "/scheduled-events/" + std::to_string(event.id), m_patch, event.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("scheduled_event", scheduled_event().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_event_get(snowflake guild_id, snowflake event_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "/scheduled-events/" + std::to_string(event_id), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("scheduled_event", scheduled_event().fill_from_json(&j), http));
		}
	});
}

};
