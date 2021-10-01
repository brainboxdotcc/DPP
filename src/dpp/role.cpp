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
#include <dpp/discordevents.h>
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
	bot_id(0)
{
}

role::~role()
{
}

role& role::fill_from_json(snowflake _guild_id, nlohmann::json* j)
{
	this->guild_id = _guild_id;
	this->name = StringNotNull(j, "name");
	this->icon = StringNotNull(j, "icon");
	this->unicode_emoji = StringNotNull(j, "unicode_emoji");
	this->id = SnowflakeNotNull(j, "id");
	this->colour = Int32NotNull(j, "color");
	this->position = Int8NotNull(j, "position");
	this->permissions = SnowflakeNotNull(j, "permissions");
	this->flags |= BoolNotNull(j, "hoist") ? dpp::r_hoist : 0;
	this->flags |= BoolNotNull(j, "managed") ? dpp::r_managed : 0;
	this->flags |= BoolNotNull(j, "mentionable") ? dpp::r_mentionable : 0;
	if (j->find("tags") != j->end()) {
		auto t = (*j)["tags"];
		this->flags |= BoolNotNull(&t, "premium_subscriber") ? dpp::r_premium_subscriber : 0;
		this->bot_id = SnowflakeNotNull(&t, "bot_id");
		this->integration_id = SnowflakeNotNull(&t, "integration_id");
	}
	return *this;
}

std::string role::build_json(bool with_id) const {
	json j;

	if (with_id) {
		j["id"] = std::to_string(id);
	}
	if (colour) {
		j["color"] = colour;
	}
	j["position"] = position;
	j["permissions"] = permissions;
	j["hoist"] = is_hoisted();
	j["mentionable"] = is_mentionable();
	j["icon"] = icon;
	j["unicode_emoji"] = unicode_emoji;

	return j.dump();
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
	return ((this->permissions & p_administrator) | (this->permissions & p_create_instant_invite));
}

bool role::has_kick_members() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_kick_members));
}

bool role::has_ban_members() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_ban_members));
}

bool role::has_administrator() const {
	return (this->permissions & p_administrator);
}

bool role::has_manage_channels() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_channels));
}

bool role::has_manage_guild() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_guild));
}

bool role::has_add_reactions() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_add_reactions));
}

bool role::has_view_audit_log() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_view_audit_log));
}

bool role::has_priority_speaker() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_priority_speaker));
}

bool role::has_stream() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_stream));
}

bool role::has_view_channel() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_view_channel));
}

bool role::has_send_messages() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_send_messages));
}

bool role::has_send_tts_messages() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_send_tts_messages));
}

bool role::has_manage_messages() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_messages));
}

bool role::has_embed_links() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_embed_links));
}

bool role::has_attach_files() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_attach_files));
}

bool role::has_read_message_history() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_read_message_history));
}

bool role::has_mention_everyone() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_mention_everyone));
}

bool role::has_use_external_emojis() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_use_external_emojis));
}

bool role::has_view_guild_insights() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_view_guild_insights));
}

bool role::has_connect() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_connect));
}

bool role::has_speak() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_speak));
}

bool role::has_mute_members() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_mute_members));
}

bool role::has_deafen_members() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_deafen_members));
}

bool role::has_move_members() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_move_members));
}

bool role::has_use_vad() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_use_vad));
}

bool role::has_change_nickname() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_change_nickname));
}

bool role::has_manage_nicknames() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_nicknames));
}

bool role::has_manage_roles() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_roles));
}

bool role::has_manage_webhooks() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_webhooks));
}

bool role::has_manage_emojis_and_stickers() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_emojis_and_stickers));
}

bool role::has_use_application_commands() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_use_application_commands));
}

bool role::has_request_to_speak() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_request_to_speak));
}

bool role::has_manage_threads() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_threads));
}

bool role::has_create_public_threads() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_create_public_threads));
}

bool role::has_create_private_threads() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_create_private_threads));
}

bool role::has_use_external_stickers() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_use_external_stickers));
}

bool role::has_send_messages_in_threads() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_send_messages_in_threads));
}

bool role::has_start_embedded_activities() const {
	return ((this->permissions & p_administrator) | (this->permissions & p_start_embedded_activities));
}
};
