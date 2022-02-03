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
#include <dpp/channel.h>
#include <dpp/cluster.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

void cluster::current_user_join_thread(snowflake thread_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/thread-members/@me", m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::current_user_leave_thread(snowflake thread_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/thread-members/@me", m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::threads_get_active(snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "/threads/active", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			thread_map threads;
			for (auto &curr_thread : j["threads"]) {
				threads[snowflake_not_null(&curr_thread, "id")] = thread().fill_from_json(&curr_thread);
			}
			callback(confirmation_callback_t("thread_map", threads, http));
		}
	});
}


void cluster::threads_get_joined_private_archived(snowflake channel_id, snowflake before_id, uint16_t limit, command_completion_event_t callback) {
	std::string parameters;
	if (before_id) {
		parameters.append("&before=" + std::to_string(before_id));
	}
	if (limit) {
		parameters.append("&limit=" + std::to_string(limit));
	}
	if (!parameters.empty()) {
		parameters[0] = '?';
	}
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "/users/@me/threads/archived/private" + parameters, m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			thread_map threads;
			for (auto &curr_thread : j["threads"]) {
				threads[snowflake_not_null(&curr_thread, "id")] = thread().fill_from_json(&curr_thread);
			}
			callback(confirmation_callback_t("thread_map", threads, http));
		}
	});
}


void cluster::threads_get_private_archived(snowflake channel_id, time_t before_timestamp, uint16_t limit, command_completion_event_t callback) {
	std::string parameters;
	if (before_timestamp) {
		parameters.append("&before=" + ts_to_string(before_timestamp));
	}
	if (limit) {
		parameters.append("&limit=" + std::to_string(limit));
	}
	if (!parameters.empty()) {
		parameters[0] = '?';
	}
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "/threads/archived/private" + parameters, m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			thread_map threads;
			for (auto &curr_thread : j["threads"]) {
				threads[snowflake_not_null(&curr_thread, "id")] = thread().fill_from_json(&curr_thread);
			}
			callback(confirmation_callback_t("thread_map", threads, http));
		}
	});
}


void cluster::threads_get_public_archived(snowflake channel_id, time_t before_timestamp, uint16_t limit, command_completion_event_t callback) {
	std::string parameters;
	if (before_timestamp) {
		parameters.append("&before=" + ts_to_string(before_timestamp));
	}
	if (limit) {
		parameters.append("&limit=" + std::to_string(limit));
	}
	if (!parameters.empty()) {
		parameters[0] = '?';
	}
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "/threads/archived/public" + parameters, m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			thread_map threads;
			for (auto &curr_thread : j["threads"]) {
				threads[snowflake_not_null(&curr_thread, "id")] = thread().fill_from_json(&curr_thread);
			}
			callback(confirmation_callback_t("thread_map", threads, http));
		}
		});
}

void cluster::thread_member_get(const snowflake thread_id, const snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/threads-members/" + std::to_string(user_id), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("thread_member", thread_member().fill_from_json(&j), http));
		}
	});
}

void cluster::thread_members_get(snowflake thread_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/threads-members", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			thread_member_map thread_members;
			for (auto& curr_member : j) {
				thread_members[snowflake_not_null(&curr_member, "user_id")] = thread_member().fill_from_json(&curr_member);
			}
			callback(confirmation_callback_t("thread_member_map", thread_members, http));
		}
	});
}


void cluster::thread_create(const std::string& thread_name, snowflake channel_id, uint16_t auto_archive_duration, channel_type thread_type, bool invitable, uint16_t rate_limit_per_user, command_completion_event_t callback)
{
	json j;
	j["name"] = thread_name;
	j["auto_archive_duration"] = auto_archive_duration;
	j["type"] = thread_type;
	j["invitable"] = invitable;
	j["rate_limit_per_user"] = rate_limit_per_user;
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "threads", m_post, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("thread", thread().fill_from_json(&j), http));
		}
	});
}


void cluster::thread_create_with_message(const std::string& thread_name, snowflake channel_id, snowflake message_id, uint16_t auto_archive_duration, uint16_t rate_limit_per_user, command_completion_event_t callback)
{
	json j;
	j["name"] = thread_name;
	j["auto_archive_duration"] = auto_archive_duration;
	j["rate_limit_per_user"] = rate_limit_per_user;
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages/" + std::to_string(message_id) + "/threads", m_post, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("thread", thread().fill_from_json(&j), http));
		}
	});
}

void cluster::thread_member_add(snowflake thread_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/thread-members/" + std::to_string(user_id), m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::thread_member_remove(snowflake thread_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/thread-members/" + std::to_string(user_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

};
