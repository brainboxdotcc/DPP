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
#include <dpp/exception.h>
#include <dpp/role.h>
#include <dpp/cache.h>
#include <dpp/discordevents.h>
#include <dpp/permissions.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

role::role() :
	managed(),
	guild_id(0),
	colour(0),
	position(0),
	permissions(0),
	flags(0),
	integration_id(0),
	bot_id(0),
	image_data(nullptr)
{
}

role::~role()
{
	if (image_data) {
		delete image_data;
	}
}

role& role::fill_from_json(nlohmann::json* j)
{
	return fill_from_json(0, j);
}

role& role::fill_from_json(snowflake _guild_id, nlohmann::json* j)
{
	this->guild_id = _guild_id;
	this->name = string_not_null(j, "name");
	this->icon = string_not_null(j, "icon");
	this->unicode_emoji = string_not_null(j, "unicode_emoji");
	this->id = snowflake_not_null(j, "id");
	this->colour = int32_not_null(j, "color");
	this->position = int8_not_null(j, "position");
	this->permissions = snowflake_not_null(j, "permissions");
	this->flags |= bool_not_null(j, "hoist") ? dpp::r_hoist : 0;
	this->flags |= bool_not_null(j, "managed") ? dpp::r_managed : 0;
	this->flags |= bool_not_null(j, "mentionable") ? dpp::r_mentionable : 0;
	if (j->contains("tags")) {
		auto t = (*j)["tags"];
		/* This is broken on the Discord API.
		 * Confirmed 25/11/2021, by quin#3017. If the value exists
		 * as a null, this is the nitro role. If it doesn't exist at all, it is
		 * NOT the nitro role. How obtuse.
		 */
		if (t.find("premium_subscriber") != t.end()) {
			this->flags |= dpp::r_premium_subscriber;
		}
		this->bot_id = snowflake_not_null(&t, "bot_id");
		this->integration_id = snowflake_not_null(&t, "integration_id");
	}
	return *this;
}

std::string role::build_json(bool with_id) const {
	json j;

	if (with_id) {
		j["id"] = std::to_string(id);
	}
	if (!name.empty()) {
		j["name"] = name;
	}
	if (colour) {
		j["color"] = colour;
	}
	j["position"] = position;
	j["permissions"] = permissions;
	j["hoist"] = is_hoisted();
	j["mentionable"] = is_mentionable();
	if (image_data) {
		j["icon"] = *image_data;
	}
	if (!unicode_emoji.empty()) {
		j["unicode_emoji"] = unicode_emoji;
	}

	return j.dump();
}

std::string role::get_mention() const {
	return "<@&" + std::to_string(id) + ">";
}

role& role::load_image(const std::string &image_blob, const image_type type) {
	static const std::map<image_type, std::string> mimetypes = {
		{ i_gif, "image/gif" },
		{ i_jpg, "image/jpeg" },
		{ i_png, "image/png" }
	};

	/* If there's already image data defined, free the old data, to prevent a memory leak */
	delete image_data;

	image_data = new std::string("data:" + mimetypes.find(type)->second + ";base64," + base64_encode((unsigned char const*)image_blob.data(), (unsigned int)image_blob.length()));

	return *this;
}


bool role::is_hoisted() const {
	return this->flags & dpp::r_hoist;
}

bool role::is_mentionable() const {
	return this->flags & dpp::r_mentionable;
}

bool role::is_managed() const {
	return this->flags & dpp::r_managed;
}

bool role::has_create_instant_invite() const {
	return has_administrator() || permissions.has(p_create_instant_invite);
}

bool role::has_kick_members() const {
	return has_administrator() || permissions.has(p_kick_members);
}

bool role::has_ban_members() const {
	return has_administrator() || permissions.has(p_ban_members);
}

bool role::has_administrator() const {
	return permissions.has(p_administrator);
}

bool role::has_manage_channels() const {
	return has_administrator() || permissions.has(p_manage_channels);
}

bool role::has_manage_guild() const {
	return has_administrator() || permissions.has(p_manage_guild);
}

bool role::has_add_reactions() const {
	return has_administrator() || permissions.has(p_add_reactions);
}

bool role::has_view_audit_log() const {
	return has_administrator() || permissions.has(p_view_audit_log);
}

bool role::has_priority_speaker() const {
	return has_administrator() || permissions.has(p_priority_speaker);
}

bool role::has_stream() const {
	return has_administrator() || permissions.has(p_stream);
}

bool role::has_view_channel() const {
	return has_administrator() || permissions.has(p_view_channel);
}

bool role::has_send_messages() const {
	return has_administrator() || permissions.has(p_send_messages);
}

bool role::has_send_tts_messages() const {
	return has_administrator() || permissions.has(p_send_tts_messages);
}

bool role::has_manage_messages() const {
	return has_administrator() || permissions.has(p_manage_messages);
}

bool role::has_embed_links() const {
	return has_administrator() || permissions.has(p_embed_links);
}

bool role::has_attach_files() const {
	return has_administrator() || permissions.has(p_attach_files);
}

bool role::has_read_message_history() const {
	return has_administrator() || permissions.has(p_read_message_history);
}

bool role::has_mention_everyone() const {
	return has_administrator() || permissions.has(p_mention_everyone);
}

bool role::has_use_external_emojis() const {
	return has_administrator() || permissions.has(p_use_external_emojis);
}

bool role::has_view_guild_insights() const {
	return has_administrator() || permissions.has(p_view_guild_insights);
}

bool role::has_connect() const {
	return has_administrator() || permissions.has(p_connect);
}

bool role::has_speak() const {
	return has_administrator() || permissions.has(p_speak);
}

bool role::has_mute_members() const {
	return has_administrator() || permissions.has(p_mute_members);
}

bool role::has_deafen_members() const {
	return has_administrator() || permissions.has(p_deafen_members);
}

bool role::has_move_members() const {
	return has_administrator() || permissions.has(p_move_members);
}

bool role::has_use_vad() const {
	return has_administrator() || permissions.has(p_use_vad);
}

bool role::has_change_nickname() const {
	return has_administrator() || permissions.has(p_change_nickname);
}

bool role::has_manage_nicknames() const {
	return has_administrator() || permissions.has(p_manage_nicknames);
}

bool role::has_manage_roles() const {
	return has_administrator() || permissions.has(p_manage_roles);
}

bool role::has_manage_webhooks() const {
	return has_administrator() || permissions.has(p_manage_webhooks);
}

bool role::has_manage_emojis_and_stickers() const {
	return has_administrator() || permissions.has(p_manage_emojis_and_stickers);
}

bool role::has_use_application_commands() const {
	return has_administrator() || permissions.has(p_use_application_commands);
}

bool role::has_request_to_speak() const {
	return has_administrator() || permissions.has(p_request_to_speak);
}

bool role::has_manage_threads() const {
	return has_administrator() || permissions.has(p_manage_threads);
}

bool role::has_create_public_threads() const {
	return has_administrator() || permissions.has(p_create_public_threads);
}

bool role::has_create_private_threads() const {
	return has_administrator() || permissions.has(p_create_private_threads);
}

bool role::has_use_external_stickers() const {
	return has_administrator() || permissions.has(p_use_external_stickers);
}

bool role::has_send_messages_in_threads() const {
	return has_administrator() || permissions.has(p_send_messages_in_threads);
}

bool role::has_use_embedded_activities() const {
	return has_administrator() || permissions.has(p_use_embedded_activities);
}

bool role::has_manage_events() const {
	return has_administrator() || permissions.has(p_manage_events);
}

bool role::has_moderate_members() const {
	return has_administrator() || permissions.has(p_moderate_members);
}

role& role::set_name(const std::string& n) {
	name = utility::validate(n, 1, 100, "Role name too short");
	return *this;
}

role& role::set_colour(uint32_t c) {
	colour = c;
	return *this;
}

role& role::set_color(uint32_t c) {
	return set_colour(c);
}

role& role::set_flags(uint8_t f) {
	flags = f;
	return *this;
}

role& role::set_integration_id(snowflake i) {
	integration_id = i;
	return *this;
}

role& role::set_bot_id(snowflake b) {
	bot_id = b;
	return *this;
}

role& role::set_guild_id(snowflake gid) {
	guild_id = gid;
	return *this;
}

members_container role::get_members() const {
	members_container gm;
	guild* g = dpp::find_guild(this->guild_id);
	if (g) {
		if (this->guild_id == this->id) {
			/* Special shortcircuit for everyone-role. Always includes all users. */
			return g->members;
		}
		for (auto & m : g->members) {
			/* Iterate all members and use std::find on their role list to see who has this role */
			auto i = std::find(m.second.roles.begin(), m.second.roles.end(), this->id);
			if (i != m.second.roles.end()) {
				gm[m.second.user_id] = m.second;
			}
		}
	}
	return gm;
}

std::string role::get_icon_url(uint16_t size) const {
	/* XXX: Discord were supposed to change their CDN over to discord.com, they haven't.
	 * At some point in the future this URL *will* change!
	 */
	if (!this->icon.to_string().empty()) {
		return utility::cdn_host + "/role-icons/" + std::to_string(this->id) + "/" + this->icon.to_string() + ".png" + utility::avatar_size(size);
	} else {
		return std::string();
	}
}


};
