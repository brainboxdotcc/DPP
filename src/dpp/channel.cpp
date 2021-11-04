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
#include <dpp/cache.h>
#include <dpp/guild.h>
#include <dpp/user.h>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

thread_member& thread_member::fill_from_json(nlohmann::json* j) {
	SetSnowflakeNotNull(j, "id", this->thread_id);
	SetSnowflakeNotNull(j, "user_id", this->user_id);
	SetTimestampNotNull(j, "join_timestamp", this->joined);
	SetInt32NotNull(j, "flags", this->flags);
	return *this;
}

void to_json(nlohmann::json& j, const thread_metadata& tmdata) {
	j["archived"] = tmdata.archived;
	j["auto_archive_duration"] = tmdata.auto_archive_duration;
	j["locked"] = tmdata.locked;
	j["invitable"] = tmdata.invitable;
}

channel::channel() :
	managed(),
	flags(0),
	guild_id(0),
	position(0),
	last_message_id(0),
	user_limit(0),
	rate_limit_per_user(0),
	owner_id(0),
	parent_id(0),
	last_pin_timestamp(0),
	message_count(0),
	member_count(0)
{
}

channel::~channel()
{
}

bool channel::is_nsfw() const {
	return flags & dpp::c_nsfw;
}

bool channel::is_text_channel() const {
	return flags & dpp::c_text;
}

bool channel::is_dm() const {
	return flags & dpp::c_dm;
}

bool channel::is_voice_channel() const {
	return flags & dpp::c_voice;
}

bool channel::is_group_dm() const {
	return (flags & (dpp::c_dm | dpp::c_group)) == (dpp::c_dm | dpp::c_group);
}

bool channel::is_category() const {
	return flags & dpp::c_category;
}

bool channel::is_stage_channel() const {
	return (flags & dpp::c_stage) == dpp::c_stage;
}

bool channel::is_news_channel() const {
	/* Important: Stage/News overlap to pack more values in a byte */
	return !is_stage_channel() && (flags & dpp::c_news);
}

bool channel::is_store_channel() const {
	/* Important: Stage/Store overlap to pack more values in a byte */
	return !is_stage_channel() && (flags & dpp::c_store);
}

bool channel::is_news_thread() const {
	return flags & dpp::c_news_thread;
}

bool channel::is_public_thread() const {
	return flags & dpp::c_public_thread;
}

bool channel::is_private_thread() const {
	return flags & dpp::c_private_thread;
}

channel& channel::fill_from_json(json* j) {
	this->id = SnowflakeNotNull(j, "id");
	SetSnowflakeNotNull(j, "guild_id", this->guild_id);
	SetInt16NotNull(j, "position", this->position);
	SetStringNotNull(j, "name", this->name);
	SetStringNotNull(j, "topic", this->topic);
	SetSnowflakeNotNull(j, "last_message_id", this->last_message_id);
	SetInt8NotNull(j, "user_limit", this->user_limit);
	SetInt16NotNull(j, "rate_limit_per_user", this->rate_limit_per_user);
	SetSnowflakeNotNull(j, "owner_id", this->owner_id);
	SetSnowflakeNotNull(j, "parent_id", this->parent_id);
	this->bitrate = Int16NotNull(j, "bitrate")/1024;
	uint8_t type = Int8NotNull(j, "type");
	this->flags |= BoolNotNull(j, "nsfw") ? dpp::c_nsfw : 0;
	this->flags |= (type == GUILD_TEXT) ? dpp::c_text : 0;
	this->flags |= (type == GUILD_VOICE) ? dpp::c_voice : 0;
	this->flags |= (type == DM) ? dpp::c_dm : 0;
	this->flags |= (type == GROUP_DM) ? (dpp::c_group | dpp::c_dm) : 0;
	this->flags |= (type == GUILD_CATEGORY) ? dpp::c_category : 0;
	this->flags |= (type == GUILD_NEWS) ? dpp::c_news : 0;
	this->flags |= (type == GUILD_STORE) ? dpp::c_store : 0;
	this->flags |= (type == GUILD_STAGE) ? dpp::c_stage : 0;
	this->flags |= (type == GUILD_NEWS_THREAD) ? dpp::c_news_thread : 0;
	this->flags |= (type == GUILD_PUBLIC_THREAD) ? dpp::c_public_thread : 0;
	this->flags |= (type == GUILD_PRIVATE_THREAD) ? dpp::c_private_thread : 0;

	if (j->find("recipients") != j->end()) {
		recipients = {};
		for (auto & r : (*j)["recipients"]) {
			recipients.push_back(from_string<uint64_t>(r["id"].get<std::string>()));
		}
	}

	if (j->find("permission_overwrites") != j->end()) {
		permission_overwrites = {};
		for (auto & overwrite : (*j)["permission_overwrites"]) {
			permission_overwrite po;
			po.id = SnowflakeNotNull(&overwrite, "id");
			po.allow = SnowflakeNotNull(&overwrite, "allow");
			po.deny = SnowflakeNotNull(&overwrite, "deny");
			po.type = Int8NotNull(&overwrite, "type");
			permission_overwrites.push_back(po);
		}
	}
	
	if (type == GUILD_NEWS_THREAD || type == GUILD_PUBLIC_THREAD || type == GUILD_PRIVATE_THREAD) {
		SetInt8NotNull(j, "message_count", this->message_count);
		SetInt8NotNull(j, "memeber_count", this->member_count);
		auto json_metadata = (*j)["thread_metadata"];
		metadata.archived = BoolNotNull(&json_metadata, "archived");
		metadata.archive_timestamp = TimestampNotNull(&json_metadata, "archive_timestamp");
		metadata.auto_archive_duration = Int16NotNull(&json_metadata, "auto_archive_duration");
		metadata.locked = BoolNotNull(&json_metadata, "locked");
	}

	return *this;
}

std::string channel::build_json(bool with_id) const {
	json j;
	if (with_id) {
		j["id"] = std::to_string(id);
	}
	j["guild_id"] = std::to_string(guild_id);
	j["position"] = position;
	j["name"] = name;
	j["topic"] = topic;
	j["rate_limit_per_user"] = rate_limit_per_user;
	if (is_voice_channel()) {
		j["user_limit"] = user_limit; 
		j["bitrate"] = bitrate*1024;
	}
	if (!is_dm()) {
		if (parent_id) {
			j["parent_id"] = parent_id;
		}
		if (is_text_channel()) {
			j["type"] = GUILD_TEXT;
		} else if (is_voice_channel()) {
			j["type"] = GUILD_VOICE;
		} else if (is_category()) {
			j["type"] = GUILD_CATEGORY;
		} else if (is_stage_channel()) {
			/* Order is important, as GUILD_STAGE overlaps NEWS and STORE */
			j["type"] = GUILD_STAGE;
		} else if (is_news_channel()) {
			j["type"] = GUILD_NEWS;
		} else if (is_store_channel()) {
			j["type"] = GUILD_STORE;
		} else if (is_news_thread()) {
			j["type"] = GUILD_NEWS_THREAD;
			j["thread_metadata"] = this->metadata;
		} else if (is_public_thread()) {
			j["type"] = GUILD_PUBLIC_THREAD;
			j["thread_metadata"] = this->metadata;
		} else if (is_private_thread()) {
			j["type"] = GUILD_PRIVATE_THREAD;
			j["thread_metadata"] = this->metadata;
		}
		j["nsfw"] = is_nsfw();
	} else {
		if (is_group_dm()) {
			j["type"] = GROUP_DM;
		} else  {
			j["type"] = DM;
		}
	}
	
	return j.dump();
}

uint64_t channel::get_user_permissions(const user* member) const
{
	if (member == nullptr)
		return 0;

	guild* g = dpp::find_guild(guild_id);
	if (g == nullptr)
		return 0;

	return g->permission_overwrites(g->base_permissions(member), member, this);
}

std::map<snowflake, guild_member*> channel::get_members() {
	std::map<snowflake, guild_member*> rv;
	guild* g = dpp::find_guild(guild_id);
	if (g) {
		for (auto m = g->members.begin(); m != g->members.end(); ++m) {
			user* u = dpp::find_user(m->second.user_id);
			if (u) {
				if (get_user_permissions(u) & p_view_channel) {
					rv[m->second.user_id] = &(m->second);
				}
			}
		}
	}
	return rv;
}

std::map<snowflake, voicestate> channel::get_voice_members() {
	std::map<snowflake, voicestate> rv;
	guild* g = dpp::find_guild(guild_id);
	if (g) {
		for (auto & m : g->voice_members) {
			if (m.second.channel_id == this->id) {
				rv[m.second.user_id] = m.second;
			}
		}
	}
	return rv;
}



};
