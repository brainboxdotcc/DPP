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
	type = (automod_action_type)int8_not_null(j, "type");
	switch (type) {
		case amod_action_send_alert:
			channel_id = snowflake_not_null(&((*j)["metadata"]), "channel_id");
			break;
		case amod_action_timeout:
			duration_seconds = int32_not_null(&((*j)["metadata"]), "duration_seconds");
			break;
		default:
			break;
	};
	return *this;
}

std::string automod_action::build_json([[maybe_unused]] bool with_id) const {
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
		default:
			break;
	};
	return j.dump();
}

automod_metadata::automod_metadata() : mention_total_limit(0) {}

automod_metadata::~automod_metadata() = default;

automod_metadata& automod_metadata::fill_from_json(nlohmann::json* j) {
	for (auto k : (*j)["keyword_filter"]) {
		keywords.push_back(k);
	}
	for (auto k : (*j)["regex_patterns"]) {
		regex_patterns.push_back(k);
	}
	for (auto k : (*j)["presets"]) {
		presets.push_back((automod_preset_type)k.get<uint32_t>());
	}
	for (auto k : (*j)["allow_list"]) {
		allow_list.push_back(k);
	}
	mention_total_limit = int8_not_null(j, "mention_total_limit");
	return *this;
}

std::string automod_metadata::build_json([[maybe_unused]] bool with_id) const {
	json j;
	j["keyword_filter"] = json::array();
	j["regex_patterns"] = json::array();
	j["presets"] = json::array();
	j["allow_list"] = json::array();
	for (auto &v : keywords) {
		j["keyword_filter"].push_back(v);
	}
	for (auto &v : regex_patterns) {
		j["regex_patterns"].push_back(v);
	}
	for (auto v : presets) {
		j["presets"].push_back((uint32_t)v);
	}
	for (auto &v : allow_list) {
		j["allow_list"].push_back(v);
	}
	j["mention_total_limit"] = mention_total_limit;
	return j.dump();

}

automod_rule::automod_rule() : managed(), guild_id(0), creator_id(0), event_type(amod_message_send), trigger_type(amod_type_keyword), enabled(true)
{
}

automod_rule::~automod_rule() = default;

automod_rule& automod_rule::fill_from_json(nlohmann::json* j) {
	id = snowflake_not_null(j, "id");
	guild_id = snowflake_not_null(j, "guild_id");
	name = string_not_null(j, "name");
	creator_id = snowflake_not_null(j, "creator_id");
	event_type = (automod_event_type)int8_not_null(j, "event_type");
	trigger_type = (automod_trigger_type)int8_not_null(j, "trigger_type");
	if (j->contains("trigger_metadata")) {
		trigger_metadata.fill_from_json(&((*j)["trigger_metadata"]));
	}
	enabled = bool_not_null(j, "enabled");
	exempt_roles.clear();
	exempt_channels.clear();
	for (auto k : (*j)["automod_actions"]) {
		actions.push_back(automod_action().fill_from_json(&k));
	}
	for (auto k : (*j)["exempt_roles"]) {
		exempt_roles.push_back(stoull(k.get<std::string>()));
	}
	for (auto k : (*j)["exempt_channels"]) {
		exempt_channels.push_back(stoull(k.get<std::string>()));
	}
	return *this;
}

std::string automod_rule::build_json(bool with_id) const {
	json j;
	if (with_id && id) {
		j["id"] = std::to_string(id);
	}
	if (guild_id) {
		j["guild_id"] = std::to_string(guild_id);
	}
	j["name"] = name;
	j["enabled"] = enabled;
	j["event_type"] = event_type;
	j["trigger_type"] = trigger_type;
	j["trigger_metadata"] = json::parse(trigger_metadata.build_json());
	if (actions.size()) {
		j["actions"] = json::array();
		json& act = j["actions"];
		for (auto v : actions) {
			act.push_back(json::parse(v.build_json()));
		}
	}
	if (exempt_roles.size()) {
		j["exempt_roles"] = json::array();
		json& roles = j["exempt_roles"];
		for (auto v : exempt_roles) {
			roles.push_back(std::to_string(v));
		}
	}
	if (exempt_channels.size()) {
		j["exempt_channels"] = json::array();
		json& channels = j["exempt_channels"];
		for (auto v : exempt_channels) {
			channels.push_back(std::to_string(v));
		}
	}
	return j.dump();
}

};

