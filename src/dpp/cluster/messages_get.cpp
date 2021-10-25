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

void cluster::messages_get(snowflake channel_id, snowflake around, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback) {
	std::string parameters;
	if (around) {
		parameters.append("&around=" + std::to_string(around));
	}
	if (before) {
		parameters.append("&before=" + std::to_string(before));
	}
	if (after) {
		parameters.append("&after=" + std::to_string(after));
	}
	if (limit) {
		parameters.append("&limit=" + std::to_string(limit));
	}
	if (!parameters.empty()) {
		parameters[0] = '?';
	}
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages" + parameters, m_get, json(), [channel_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			message_map messages;
			for (auto & curr_message : j) {
				messages[SnowflakeNotNull(&curr_message, "id")] = message().fill_from_json(&curr_message);
			}
			callback(confirmation_callback_t("message_map", messages, http));
		}
	});
}

};