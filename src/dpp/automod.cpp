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
#include <dpp/automod.h>
#include <dpp/discordevents.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

automod_action::automod_action() : channel_id(0), duration_seconds(0)
{
}

automod_action::~automod_action() = default;

automod_action& automod_action::fill_from_json(nlohmann::json* j) {
	type = (automod_action_type)uint8_not_null(j, "type");
	switch (type) {
		case amod_action_send_alert:
			channel_id = snowflake_not_null(&((*j)["metadata"]), "channel_id");
			break;
		case amod_action_timeout:
			duration_seconds = int32_not_null(&((*j)["metadata"]), "duration_seconds");
			break;
	}
	if (j->contains("user")) {
		json & user = (*j)["user"];
		user_id = snowflake_not_null(&user, "id");
	}
	return *this;
}

std::string automod_action::build_json(bool with_id) const {
	json j({
		{ "type", type }
	});
	switch (type) {
		case amod_action_send_alert:
			if (channel_id) {
				j["metadata"] = json::object();
				j["metadata"]["channel_id"] = std::to_string(channel_id);
			}
			break;
		case amod_action_timeout:
			if (duration_seconds) {
				j["metadata"] = json::object();
				j["metadata"]["duration_seconds"] = duration_seconds;
			}
			break;
	}
	return j.dump();
}

automod_metadata::~automod_metadata() = default;

automod_metadata& automod_metadata::fill_from_json(nlohmann::json* j) {
	return *this;
}

std::string automod_metadata::build_json(bool with_id) const {
	return "{}";
}

automod_rule::automod_rule() : managed(), guild_id(0), creator_id(0), event_type(amod_message_send), trigger_type(amod_type_keyword), enabled(true)
{
}

automod_action::~automod_action() = default;

automod_action& ban::fill_from_json(nlohmann::json* j) {
	return *this;
}

std::string automod_action::build_json(bool with_id) const {
	return "{}";
}

};

