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
#include <dpp/user.h>
#include <dpp/cluster.h>
#include <dpp/exception.h>
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
			throw dpp::length_exception("User icon file exceeds discord limit of 256 kilobytes");
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
			callback(confirmation_callback_t("user_identified", user_identified().fill_from_json(&j), http));
		}
	});
}

void cluster::current_user_set_voice_state(snowflake guild_id, snowflake channel_id, bool suppress, time_t request_to_speak_timestamp, command_completion_event_t callback) {
	json j({
		{"channel_id", channel_id},
		{"suppress", suppress}
	});
	if (request_to_speak_timestamp) {
		if (request_to_speak_timestamp < time(nullptr)) {
			throw dpp::logic_exception("Cannot set voice state request to speak timestamp to before current time");
		}
		j["request_to_speak_timestamp"] = ts_to_string(request_to_speak_timestamp);
	} else {
		j["request_to_speak_timestamp"] = json::value_t::null;
	}
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "/voice-states/@me", m_patch, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::user_set_voice_state(snowflake user_id, snowflake guild_id, snowflake channel_id, bool suppress, command_completion_event_t callback) {
	json j({
		{"channel_id", channel_id},
		{"suppress", suppress}
	});
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "/voice-states/" + std::to_string(user_id), m_patch, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::current_user_connections_get(command_completion_event_t callback) {
	this->post_rest(API_PATH "/users", "@me", "connections", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			connection_map connections;
			confirmation_callback_t e("confirmation", confirmation(), http);
			if (!e.is_error()) {
				for (auto & curr_conn : j) {
					connections[snowflake_not_null(&curr_conn, "id")] = connection().fill_from_json(&curr_conn);
				}
			}
			callback(confirmation_callback_t("connection_map", connections, http));
		}
	});
}

void cluster::current_user_get_guilds(command_completion_event_t callback) {
	this->post_rest(API_PATH "/users", "@me", "guilds", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			guild_map guilds;
			confirmation_callback_t e("confirmation", confirmation(), http);
			if (!e.is_error()) {
				for (auto & curr_guild : j) {
					guilds[snowflake_not_null(&curr_guild, "id")] = guild().fill_from_json(nullptr, &curr_guild);
				}
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
			callback(confirmation_callback_t("user_identified", user_identified().fill_from_json(&j), http));
		}
	});
}

};