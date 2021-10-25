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
#include <map>
#include <dpp/discord.h>
#include <dpp/cluster.h>
#include <dpp/discordclient.h>
#include <dpp/discordevents.h>
#include <dpp/message.h>
#include <dpp/cache.h>
#include <dpp/nlohmann/json.hpp>
#include <utility>

namespace dpp {

void cluster::get_thread_members(snowflake thread_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/threads-members", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			thread_member_map thread_members;
			for (auto& curr_member : j) {
				thread_members[SnowflakeNotNull(&curr_member, "user_id")] = thread_member().fill_from_json(&curr_member);
			}
			callback(confirmation_callback_t("thread_member_map", thread_members, http));
		}
	});
}

};