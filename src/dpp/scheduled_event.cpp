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
#include <dpp/scheduled_event.h>
#include <dpp/discord.h>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

scheduled_event::scheduled_event() :
	id(0),
	guild_id(0),
	channel_id(0),	
	creator_id(0),	
	scheduled_start_time(0),
	scheduled_end_time(0),
	privacy_level(ep_public),
	status(es_scheduled),
	entity_type(eet_none),
	entity_id(0),
	user_count(0)
{
}

scheduled_event& scheduled_event::fill_from_json(const json* j) {
	SetSnowflakeNotNull(j, "id", this->id);
	SetSnowflakeNotNull(j, "guild_id", this->guild_id);
	SetSnowflakeNotNull(j, "channel_id", this->channel_id);
	SetSnowflakeNotNull(j, "creator_id", this->creator_id);
	SetSnowflakeNotNull(j, "creator_id", this->creator_id);
	SetStringNotNull(j, "name", this->name);
	SetStringNotNull(j, "description", this->description);
	SetStringNotNull(j, "image", this->image);
	SetTimestampNotNull(j, "scheduled_start_time", this->scheduled_start_time);
	SetTimestampNotNull(j, "scheduled_end_time", this->scheduled_end_time);
	this->privacy_level = static_cast<dpp::event_privacy_level>(Int8NotNull(j, "privacy_level"));
	this->status = static_cast<dpp::event_status>(Int8NotNull(j, "status"));
	this->entity_type = static_cast<dpp::event_entity_type>(Int8NotNull(j, "entity_type"));
	auto i = j->find("entity_metadata");
	if (i != j->end()) {
		SetStringNotNull(&((*j)["entity_metadata"]), "location", this->entity_metadata.location);
		if (i->find("speaker_ids") != i->end()) {
			for (auto & speaker : (*j)["entity_metadata"]["speaker_ids"]) {
				this->entity_metadata.speaker_ids.push_back(std::stoull(speaker.get<std::string>()));
			}
		}
	}
	if (j->find("creator") != j->end()) {
		json u = (*j)["creator"];
		creator.fill_from_json(&u);
	}
	SetInt32NotNull(j, "user_count", this->user_count);
	return *this;
}

std::string const scheduled_event::build_json(bool with_id) const {
	json j;
	if (this->id && with_id) {
		j["id"] = std::to_string(id);
	}
	j["name"] = this->name;
	if (!this->description.empty()) {
		j["description"] = this->description;
	}
	if (!this->image.empty()) {
		j["image"] = this->image;
	}
	j["privacy_level"] = this->privacy_level;
	j["status"] = this->status;
	j["entity_type"] = this->entity_type;
	if (this->entity_id) {
		j["entity_id"] = std::to_string(this->entity_id);
	}
	if (this->channel_id) {
		j["channel_id"] = std::to_string(this->channel_id);
	}
	if (this->guild_id) {
		j["guild_id"] = std::to_string(this->guild_id);
	}
	if (this->creator_id) {
		j["creator_id"] = std::to_string(this->creator_id);
	}
	if (scheduled_start_time) {
		std::ostringstream ss;
		struct tm t;
		#ifdef _WIN32
			gmtime_s(&t, &scheduled_start_time);
		#else
			gmtime_r(&scheduled_start_time, &t);
		#endif
		ss << std::put_time(&t, "%FT%TZ");
		j["scheduled_start_time"] = ss.str();
	}
	if (scheduled_end_time) {
		std::ostringstream ss;
		struct tm t;
		#ifdef _WIN32
			gmtime_s(&t, &scheduled_end_time);
		#else
			gmtime_r(&scheduled_end_time, &t);
		#endif
		ss << std::put_time(&t, "%FT%TZ");
		j["scheduled_end_time"] = ss.str();
	}
	if (!entity_metadata.location.empty() || entity_metadata.speaker_ids.size()) {
		j["entity_metadata"] = json::object();
		if (!entity_metadata.location.empty()) {
			j["entity_metadata"]["location"] = entity_metadata.location;
		}
		if (entity_metadata.speaker_ids.size()) {
			j["entity_metadata"]["speaker_ids"] = json::array();
			for (auto & s : entity_metadata.speaker_ids) {
				j["entity_metadata"]["speaker_ids"].push_back(std::to_string(s));
			}
		}
	}

	return j.dump();
}

};
