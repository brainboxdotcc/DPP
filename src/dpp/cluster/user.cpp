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

void cluster::current_user_edit(const std::string &nickname, const std::string& image_blob, const image_type type, command_completion_event_t callback) {
	json j = json::parse("{\"nickname\": null}");
	if (!nickname.empty()) {
		j["nickname"] = nickname;
	}
	if (!image_blob.empty()) {
		static const std::map<image_type, std::string> mimetypes = {
			{ i_gif, "image/gif" },
			{ i_jpg, "image/jpeg" },
			{ i_png, "image/png" }
		};
		if (image_blob.size() > MAX_EMOJI_SIZE) {
			throw dpp::exception("User icon file exceeds discord limit of 256 kilobytes");
		}
		j["avatar"] = "data:" + mimetypes.find(type)->second + ";base64," + base64_encode((unsigned char const*)image_blob.data(), (unsigned int)image_blob.length());
	}
	this->post_rest(API_PATH "/users", "@me", "", m_patch, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("user", user().fill_from_json(&j), http));
		}
	});
}


void cluster::current_application_get(command_completion_event_t callback) {
	this->post_rest(API_PATH "/oauth2/applications", "@me", "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("application", application().fill_from_json(&j), http));
		}
	});
}

void cluster::current_user_get(command_completion_event_t callback) {
	this->post_rest(API_PATH "/users", "@me", "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("user", user().fill_from_json(&j), http));
		}
	});
}

void cluster::current_user_connections_get(command_completion_event_t callback) {
	this->post_rest(API_PATH "/users", "@me", "connections", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			connection_map connections;
			for (auto & curr_conn : j) {
				connections[SnowflakeNotNull(&curr_conn, "id")] = connection().fill_from_json(&curr_conn);
			}
			callback(confirmation_callback_t("connection_map", connections, http));
		}
	});
}

void cluster::current_user_get_guilds(command_completion_event_t callback) {
	this->post_rest(API_PATH "/users", "@me", "guilds", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			guild_map guilds;
			for (auto & curr_guild : j) {
				guilds[SnowflakeNotNull(&curr_guild, "id")] = guild().fill_from_json(nullptr, &curr_guild);
			}
			callback(confirmation_callback_t("guild_map", guilds, http));
		}
	});
}


void cluster::current_user_leave_guild(snowflake guild_id, command_completion_event_t callback) {
	 this->post_rest(API_PATH "/users", "@me", "guilds/" + std::to_string(guild_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	 });
}


void cluster::user_get(snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/users", std::to_string(user_id), "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("user", user().fill_from_json(&j), http));
		}
	});
}

};