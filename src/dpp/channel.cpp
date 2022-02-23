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
#include <dpp/cache.h>
#include <dpp/guild.h>
#include <dpp/user.h>
#include <dpp/role.h>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

thread_member& thread_member::fill_from_json(nlohmann::json* j) {
	set_snowflake_not_null(j, "id", this->thread_id);
	set_snowflake_not_null(j, "user_id", this->user_id);
	set_ts_not_null(j, "join_timestamp", this->joined);
	set_int32_not_null(j, "flags", this->flags);
	return *this;
}

void to_json(nlohmann::json& j, const thread_metadata& tmdata) {
	j["archived"] = tmdata.archived;
	j["auto_archive_duration"] = tmdata.auto_archive_duration;
	j["locked"] = tmdata.locked;
	j["invitable"] = tmdata.invitable;
}

void to_json(nlohmann::json& j, const permission_overwrite& po) {
	j["id"] = std::to_string(po.id);
	j["allow"] = std::to_string(po.allow);
	j["deny"] = std::to_string(po.deny);
	j["type"] = po.type;
}

channel::channel() :
	managed(),
	flags(0),
	guild_id(0),
	position(0),
	last_message_id(0),
	user_limit(0),
	bitrate(0),
	rate_limit_per_user(0),
	owner_id(0),
	parent_id(0),
	last_pin_timestamp(0),
	permissions(0)
{
}

channel::~channel()
{
}

std::string channel::get_mention() const {
	return "<#" + std::to_string(id) + ">";
}

channel& channel::set_name(const std::string& name) {
	this->name = utility::validate(name, 1, 100, "name must be at least 1 character");
	return *this;
}

channel& channel::set_topic(const std::string& topic) {
	this->topic = utility::utf8substr(topic, 0, 1024);
	return *this;
}

channel& channel::set_parent_id(const snowflake parent_id) {
	this->parent_id = parent_id;
	return *this;
}

channel& channel::set_rate_limit_per_user(const uint16_t rate_limit_per_user) {
	this->rate_limit_per_user = rate_limit_per_user;
	return *this;
}

channel& channel::set_position(const uint16_t position) {
	this->position = position;
	return *this;
}

channel& channel::set_bitrate(const uint16_t bitrate) {
	this->bitrate = bitrate;
	return *this;
}

channel& channel::set_flags(const uint16_t flags) {
	this->flags = flags;
	return *this;
}

channel& channel::add_flag(const channel_flags flag) {
	this->flags |= flag;
	return *this;
}

channel& channel::remove_flag(const channel_flags flag) {
	this->flags &= ~flag;
	return *this;
}

channel& channel::set_nsfw(const bool is_nsfw) {
	this->flags = (is_nsfw) ? this->flags | dpp::c_nsfw : this->flags & ~dpp::c_nsfw;;
	return *this;
}

channel& channel::set_user_limit(const uint8_t user_limit) {
	this->user_limit = user_limit;
	return *this;
}

channel& channel::add_permission_overwrite(const snowflake id, const uint8_t type, const uint64_t allowed_permissions, const uint64_t denied_permissions) {
	permission_overwrite po {id, type, allowed_permissions, denied_permissions};
	this->permission_overwrites.push_back(po);
	return *this;
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

bool channel::is_video_auto() const {
	/* Note: c_video_auto has no real flag (its value is 0)
	 * as absence of the 720p FULL quality flag indicates it must be
	 * c_video_auto instead -- discord decided to put what is basically
	 * a bool into two potential values, 1 and 2. hmmm...
	 */
	return !is_video_720p();
}

bool channel::is_video_720p() const {
	return (flags & dpp::c_video_quality_720p);
}


bool thread::is_news_thread() const {
	return flags & dpp::c_news_thread;
}

bool thread::is_public_thread() const {
	return flags & dpp::c_public_thread;
}

bool thread::is_private_thread() const {
	return flags & dpp::c_private_thread;
}

thread& thread::fill_from_json(json* j) {
	channel::fill_from_json(j);

	uint8_t type = int8_not_null(j, "type");
	this->flags |= (type == GUILD_NEWS_THREAD) ? dpp::c_news_thread : 0;
	this->flags |= (type == GUILD_PUBLIC_THREAD) ? dpp::c_public_thread : 0;
	this->flags |= (type == GUILD_PRIVATE_THREAD) ? dpp::c_private_thread : 0;

	set_int8_not_null(j, "message_count", this->message_count);
	set_int8_not_null(j, "member_count", this->member_count);
	auto json_metadata = (*j)["thread_metadata"];
	metadata.archived = bool_not_null(&json_metadata, "archived");
	metadata.archive_timestamp = ts_not_null(&json_metadata, "archive_timestamp");
	metadata.auto_archive_duration = int16_not_null(&json_metadata, "auto_archive_duration");
	metadata.locked = bool_not_null(&json_metadata, "locked");

	/* Only certain events set this */
	if (j->find("member") != j->end())  {
		member.fill_from_json(&((*j)["member"]));
	}
	
	return *this;
}

thread::thread() : channel(), message_count(0), member_count(0) {
}

thread::~thread() {
}

channel& channel::fill_from_json(json* j) {
	this->id = snowflake_not_null(j, "id");
	set_snowflake_not_null(j, "guild_id", this->guild_id);
	set_int16_not_null(j, "position", this->position);
	set_string_not_null(j, "name", this->name);
	set_string_not_null(j, "topic", this->topic);
	set_snowflake_not_null(j, "last_message_id", this->last_message_id);
	set_int8_not_null(j, "user_limit", this->user_limit);
	set_int16_not_null(j, "rate_limit_per_user", this->rate_limit_per_user);
	set_snowflake_not_null(j, "owner_id", this->owner_id);
	set_snowflake_not_null(j, "parent_id", this->parent_id);
	this->bitrate = int16_not_null(j, "bitrate")/1024;
	uint8_t type = int8_not_null(j, "type");
	this->flags |= bool_not_null(j, "nsfw") ? dpp::c_nsfw : 0;
	this->flags |= (type == GUILD_TEXT) ? dpp::c_text : 0;
	this->flags |= (type == GUILD_VOICE) ? dpp::c_voice : 0;
	this->flags |= (type == DM) ? dpp::c_dm : 0;
	this->flags |= (type == GROUP_DM) ? (dpp::c_group | dpp::c_dm) : 0;
	this->flags |= (type == GUILD_CATEGORY) ? dpp::c_category : 0;
	this->flags |= (type == GUILD_NEWS) ? dpp::c_news : 0;
	this->flags |= (type == GUILD_STORE) ? dpp::c_store : 0;
	this->flags |= (type == GUILD_STAGE) ? dpp::c_stage : 0;

	uint8_t vqm = int8_not_null(j, "video_quality_mode");
	if (vqm == 2) {
		/* If this is set to 2, this means full quality 720p video for voice channel.
		 * Undefined, or a value of 1 (the other two possibilities right now) means
		 * video quality AUTO.
		 */
		this->flags |= dpp::c_video_quality_720p;
	}

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
			po.id = snowflake_not_null(&overwrite, "id");
			po.allow = snowflake_not_null(&overwrite, "allow");
			po.deny = snowflake_not_null(&overwrite, "deny");
			po.type = int8_not_null(&overwrite, "type");
			permission_overwrites.emplace_back(po);
		}
	}

	/* Note: This is only set when the channel is in the resolved set from an interaction.
	 * When set it contains the invokers permissions on channel. Any other time, contains 0.
	 */
	if (j->find("permissions") != j->end()) {
		set_snowflake_not_null(j, "permissions", permissions);
	}

	std::string _icon = string_not_null(j, "icon");
	std::string _banner = string_not_null(j, "banner");

	if (!_icon.empty()) {
		this->icon = _icon;
	}

	if (!_banner.empty()) {
		this->banner = _banner;
	}

	set_string_not_null(j, "rtc_region", rtc_region);
	
	return *this;
}

std::string thread::build_json(bool with_id) const {
	json j = json::parse(channel::build_json(with_id));
	if (is_news_thread()) {
		/* News thread */
		j["type"] = GUILD_NEWS_THREAD;
		j["thread_metadata"] = this->metadata;
	} else if (is_public_thread()) {
		/* Public thread */
		j["type"] = GUILD_PUBLIC_THREAD;
		j["thread_metadata"] = this->metadata;
	} else {
		/* Private thread */
		j["type"] = GUILD_PRIVATE_THREAD;
		j["thread_metadata"] = this->metadata;
	}
	return j.dump();
}

std::string channel::build_json(bool with_id) const {
	json j;
	if (with_id && id) {
		j["id"] = std::to_string(id);
	}
	j["guild_id"] = std::to_string(guild_id);
	if (position) {
		j["position"] = position;
	}
	j["name"] = name;
	if (!topic.empty()) {
		j["topic"] = topic;
	}
	if (!permission_overwrites.empty()) {
		j["permission_overwrites"] = json::array();
		for (const auto& po : permission_overwrites) {
			json jpo = po;
			j["permission_overwrites"].push_back(jpo);
		}
	}
	if (rate_limit_per_user) {
		j["rate_limit_per_user"] = rate_limit_per_user;
	}
	if (is_voice_channel()) {
		j["user_limit"] = user_limit; 
		j["bitrate"] = bitrate*1024;
	}
	if (!is_dm()) {
		if (parent_id) {
			j["parent_id"] = std::to_string(parent_id);
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

std::string channel::get_banner_url(uint16_t size) const {
	/* XXX: Discord were supposed to change their CDN over to discord.com, they haven't.
	 * At some point in the future this URL *will* change!
	 */
	if (!this->banner.to_string().empty()) {
		// TODO implement this, endpoint for that isn't finished yet https://discord.com/developers/docs/reference#image-formatting-cdn-endpoints
		return std::string();
	} else {
		return std::string();
	}
}

std::string channel::get_icon_url(uint16_t size) const {
	/* XXX: Discord were supposed to change their CDN over to discord.com, they haven't.
	 * At some point in the future this URL *will* change!
	 */
	if (!this->icon.to_string().empty()) {
		// TODO implement this, endpoint for that isn't finished yet https://discord.com/developers/docs/reference#image-formatting-cdn-endpoints
		return std::string();
	} else {
		return std::string();
	}
}


};
