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
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

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
	last_pin_timestamp(0)
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

channel& channel::fill_from_json(json* j) {
	this->id = SnowflakeNotNull(j, "id");
	this->guild_id = SnowflakeNotNull(j, "guild_id");
	this->position = Int16NotNull(j, "position");
	this->name = StringNotNull(j, "name");
	this->topic = StringNotNull(j, "topic");
	this->last_message_id = SnowflakeNotNull(j, "last_message_id");
	this->user_limit = Int8NotNull(j, "user_limit");
	this->rate_limit_per_user = Int16NotNull(j, "rate_limit_per_user");
	this->owner_id = SnowflakeNotNull(j, "owner_id");
	this->parent_id = SnowflakeNotNull(j, "parent_id");
	//this->last_pin_timestamp
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

	if (j->find("recipients") != j->end()) {
		recipients.clear();
		for (auto & r : (*j)["recipients"]) {
			recipients.push_back(from_string<uint64_t>(r["id"].get<std::string>(), std::dec));
		}
	}

	if (j->find("permission_overwrites") != j->end()) {
		for (auto & overwrite : (*j)["permission_overwrites"]) {
			permission_overwrite po;
			po.id = SnowflakeNotNull(&overwrite, "id");
			po.allow = SnowflakeNotNull(&overwrite, "allow");
			po.deny = SnowflakeNotNull(&overwrite, "deny");
			po.type = Int8NotNull(&overwrite, "type");
			permission_overwrites.push_back(po);
		}
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
	if (is_voice_channel()) {
		j["user_limit"] = user_limit; 
		j["rate_limit_per_user"] = rate_limit_per_user;
	}
	if (!is_dm()) {
		j["parent_id"] = parent_id;
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



};
