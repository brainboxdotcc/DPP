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
#pragma once

#include <dpp/json_fwd.hpp>

namespace dpp {

/** Various flags related to dpp::role */
enum role_flags {
	r_hoist =		0b00000001, //< Hoisted role
	r_managed =		0b00000010, //< Managed role (introduced by a bot or application)
	r_mentionable =		0b00000100, //< Mentionable with an @ping
	r_premium_subscriber =	0b00001000, //< This is set for the role given to nitro 
};

/**
 * @brief Represents the various discord permissions
 */
enum role_permissions {
	p_create_instant_invite	=	0x00000001,	//< allows creationboosters of instant invites
	p_kick_members		=	0x00000002,	//< allows kicking members
	p_ban_members		=	0x00000004,	//< allows banning members
	p_administrator		=	0x00000008,	//< allows all permissions and bypasses channel permission overwrites
	p_manage_channels	=	0x00000010,	//< allows management and editing of channels
	p_manage_guild		=	0x00000020,	//< allows management and editing of the guild
	p_add_reactions		=	0x00000040,	//< allows for the addition of reactions to messages
	p_view_audit_log	=	0x00000080,	//< allows for viewing of audit logs
	p_priority_speaker	=	0x00000100,	//< allows for using priority speaker in a voice channel
	p_stream		=	0x00000200,	//< allows the user to go live
	p_view_channel		=	0x00000400,	//< allows guild members to view a channel, which includes reading messages in text channels
	p_send_messages		=	0x00000800,	//< allows for sending messages in a channel
	p_send_tts_messages	=	0x00001000,	//< allows for sending of /tts messages
	p_manage_messages	=	0x00002000,	//< allows for deletion of other users messages
	p_embed_links		=	0x00004000,	//< links sent by users with this permission will be auto-embedded
	p_attach_files		=	0x00008000,	//< allows for uploading images and files
	p_read_message_history	=	0x00010000,	//< allows for reading of message history
	p_mention_everyone	=	0x00020000,	//< allows for using the @everyone and the @here tag to notify users in a channel
	p_use_external_emojis	=	0x00040000,	//< allows the usage of custom emojis from other servers
	p_view_guild_insights	=	0x00080000,	//< allows for viewing guild insights
	p_connect		=	0x00100000,	//< allows for joining of a voice channel
	p_speak			=	0x00200000,	//< allows for speaking in a voice channel
	p_mute_members		=	0x00400000,	//< allows for muting members in a voice channel
	p_deafen_members	=	0x00800000,	//< allows for deafening of members in a voice channel
	p_move_members		=	0x01000000,	//< allows for moving of members between voice channels
	p_use_vad		=	0x02000000,	//< allows for using voice-activity-detection in a voice channel
	p_change_nickname	=	0x04000000,	//< allows for modification of own nickname 
	p_manage_nicknames	=	0x08000000,	//< allows for modification of other users nicknames 
	p_manage_roles		=	0x10000000,	//< allows management and editing of roles 
	p_manage_webhooks	=	0x20000000,	//< allows management and editing of webhooks
	p_manage_emojis		=	0x40000000	//< allows management and editing of emojis 
};

/**
 * @brief Represents a role within a dpp::guild
 */
class role : public managed {
public:
	/** Role name */
	std::string name;
	/** Guild id */
	snowflake guild_id;
	/** Role colour */
	uint32_t colour;
	/** Role position */
	uint8_t position;
	/** Role permissions bitmask values from dpp::role_permissions */
	uint32_t permissions;
	/** Role flags from dpp::role_flags */
	uint8_t flags;
	/** Integration id if any (e.g. role is a bot's role created when it was invited) */
	snowflake integration_id;
	/** Bot id if any (e.g. role is a bot's role created when it was invited) */
	snowflake bot_id;

	/** Default constructor */
	role();

	/** Default destructor */
	~role();

	/** Fill this role from json.
	 * @param guild_id the guild id to place in the json
	 * @param j The json data
	 * @return A reference to self
	 */
	role& fill_from_json(snowflake guild_id, nlohmann::json* j);

	/** Build a json string from this object.
	 * @param with_id true if the ID is to be included in the json text
	 * @return The json of the role
	 */
	std::string build_json(bool with_id = false) const;

	/** True if the role is hoisted */
	bool is_hoisted() const;
	/** True if the role is mentionable */
	bool is_mentionable() const;
	/** True if the role is managed (belongs to a bot or application) */
	bool is_managed() const;
	/** True if has create instant invite permission */
	bool has_create_instant_invite() const;
	/** True if has the kick members permission */
	bool has_kick_members() const;
	/** True if has the ban members permission */
	bool has_ban_members() const;
	/** True if has the administrator permission */
	bool has_administrator() const;
	/** True if has the manage channels permission */
	bool has_manage_channels() const;
	/** True if has the manage guild permission */
	bool has_manage_guild() const;
	/** True if has the add reactions permission */
	bool has_add_reactions() const;
	/** True if has the view audit log permission */
	bool has_view_audit_log() const;
	/** True if has the priority speaker permission */
	bool has_priority_speaker() const;
	/** True if has the stream permission */
	bool has_stream() const;
	/** True if has the view channel permission */
	bool has_view_channel() const;
	/** True if has the send messages permission */
	bool has_send_messages() const;
	/** True if has the send TTS messages permission */
	bool has_send_tts_messages() const;
	/** True if has the manage messages permission */
	bool has_manage_messages() const;
	/** True if has the embed links permission */
	bool has_embed_links() const;
	/** True if has the attach files permission */
	bool has_attach_files() const;
	/** True if has the read message history permission */
	bool has_read_message_history() const;
	/** True if has the mention \@everyone and \@here permission */
	bool has_mention_everyone() const;
	/** True if has the use external emojis permission */
	bool has_use_external_emojis() const;
	/** True if has the view guild insights permission */
	bool has_view_guild_insights() const;
	/** True if has the connect voice permission */
	bool has_connect() const;
	/** True if has the speak permission */
	bool has_speak() const;
	/** True if has the mute members permission */
	bool has_mute_members() const;
	/** True if has the deafen members permission */
	bool has_deafen_members() const;
	/** True if has the move members permission */
	bool has_move_members() const;
	/** True if has use voice activity detection permission */
	bool has_use_vad() const;
	/** True if has the change nickname permission */
	bool has_change_nickname() const;
	/** True if has the manage nicknames permission */
	bool has_manage_nicknames() const;
	/** True if has the manage roles permission */
	bool has_manage_roles() const;
	/** True if has the manage webhooks permission */
	bool has_manage_webhooks() const;
	/** True if has the manage emojis permission */
	bool has_manage_emojis() const;
};

/** A group of roles */
typedef std::unordered_map<snowflake, role> role_map;

};

