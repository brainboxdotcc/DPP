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
#include <dpp/export.h>
#include <dpp/managed.h>
#include <dpp/json_fwd.hpp>
#include <dpp/guild.h>

namespace dpp {

/** Various flags related to dpp::role */
enum role_flags : uint8_t {
	r_hoist =		0b00000001, //!< Hoisted role
	r_managed =		0b00000010, //!< Managed role (introduced by a bot or application)
	r_mentionable =		0b00000100, //!< Mentionable with a ping
	r_premium_subscriber =	0b00001000, //!< This is set for the role given to nitro 
};

/**
 * @brief Represents the various discord permissions
 */
enum role_permissions : uint64_t {
	p_create_instant_invite		=	0x00000000001,	//!< allows creation of instant invites
	p_kick_members			=	0x00000000002,	//!< allows kicking members
	p_ban_members			=	0x00000000004,	//!< allows banning members
	p_administrator			=	0x00000000008,	//!< allows all permissions and bypasses channel permission overwrites
	p_manage_channels		=	0x00000000010,	//!< allows management and editing of channels
	p_manage_guild			=	0x00000000020,	//!< allows management and editing of the guild
	p_add_reactions			=	0x00000000040,	//!< allows for the addition of reactions to messages
	p_view_audit_log		=	0x00000000080,	//!< allows for viewing of audit logs
	p_priority_speaker		=	0x00000000100,	//!< allows for using priority speaker in a voice channel
	p_stream			=	0x00000000200,	//!< allows the user to go live
	p_view_channel			=	0x00000000400,	//!< allows guild members to view a channel, which includes reading messages in text channels and joining voice channels
	p_send_messages			=	0x00000000800,	//!< allows for sending messages in a channel
	p_send_tts_messages		=	0x00000001000,	//!< allows for sending of /tts messages
	p_manage_messages		=	0x00000002000,	//!< allows for deletion of other users messages
	p_embed_links			=	0x00000004000,	//!< links sent by users with this permission will be auto-embedded
	p_attach_files			=	0x00000008000,	//!< allows for uploading images and files
	p_read_message_history		=	0x00000010000,	//!< allows for reading of message history
	p_mention_everyone		=	0x00000020000,	//!< allows for using the everyone and the here tag to notify users in a channel
	p_use_external_emojis		=	0x00000040000,	//!< allows the usage of custom emojis from other servers
	p_view_guild_insights		=	0x00000080000,	//!< allows for viewing guild insights
	p_connect			=	0x00000100000,	//!< allows for joining of a voice channel
	p_speak				=	0x00000200000,	//!< allows for speaking in a voice channel
	p_mute_members			=	0x00000400000,	//!< allows for muting members in a voice channel
	p_deafen_members		=	0x00000800000,	//!< allows for deafening of members in a voice channel
	p_move_members			=	0x00001000000,	//!< allows for moving of members between voice channels
	p_use_vad			=	0x00002000000,	//!< allows for using voice-activity-detection in a voice channel
	p_change_nickname		=	0x00004000000,	//!< allows for modification of own nickname 
	p_manage_nicknames		=	0x00008000000,	//!< allows for modification of other users nicknames 
	p_manage_roles			=	0x00010000000,	//!< allows management and editing of roles 
	p_manage_webhooks		=	0x00020000000,	//!< allows management and editing of webhooks
	p_manage_emojis_and_stickers	=	0x00040000000,	//!< allows management and editing of emojis and stickers
	p_use_application_commands	=	0x00080000000,	//!< allows members to use application commands, including slash commands and context menus 
	p_request_to_speak		=	0x00100000000,	//!< allows for requesting to speak in stage channels. (Discord: This permission is under active development and may be changed or removed.)
	p_manage_events			=	0x00200000000,	//!< allows for management (creation, updating, deleting, starting) of scheduled events
	p_manage_threads		=	0x00400000000,	//!< allows for deleting and archiving threads, and viewing all private threads
	p_create_public_threads		=	0x00800000000,	//!< allows for creating public and announcement threads
	p_create_private_threads	=	0x01000000000,	//!< allows for creating private threads
	p_use_external_stickers		=	0x02000000000,	//!< allows the usage of custom stickers from other servers
	p_send_messages_in_threads	=	0x04000000000,	//!< allows for sending messages in threads
	p_start_embedded_activities	=	0x08000000000,	//!< allows for launching activities (applications with the EMBEDDED flag) in a voice channel
	p_moderate_members		=	0x10000000000,	//!< allows for timing out users to prevent them from sending or reacting to messages in chat and threads, and from speaking in voice and stage channels
};

/**
 * @brief Represents a role within a dpp::guild.
 * Roles are combined via logical OR of the permission bitmasks, then channel-specific overrides
 * can be applied on top, deny types apply a logic NOT to the bit mask, and allows apply a logical OR.
 * @note Every guild has at least one role, called the 'everyone' role, which always has the same role
 * ID as the guild's ID. This is the base permission set applied to all users where no other role or override
 * applies, and is the starting value of the bit mask looped through to calculate channel permissions.
 */
class DPP_EXPORT role : public managed {
public:
	/**
	 * @brief Role name
	 * Between 1 and 100 characters.
	 */
	std::string name;
	/**
	 * @brief Guild ID
	 */
	snowflake guild_id;
	/**
	 * @brief Role colour.
	 * A colour of 0 means no colour. If you want a black role,
	 * you must use the value 0x000001.
	 */
	uint32_t colour;
	/** Role position */
	uint8_t position;
	/** Role permissions bitmask values from dpp::role_permissions */
	uint64_t permissions;
	/** Role flags from dpp::role_flags */
	uint8_t flags;
	/** Integration id if any (e.g. role is a bot's role created when it was invited) */
	snowflake integration_id;
	/** Bot id if any (e.g. role is a bot's role created when it was invited) */
	snowflake bot_id;
	/** The unicode emoji used for the role's icon, can be an empty string */
	std::string unicode_emoji;
	/** The role icon hash, can be an empty string */
	utility::iconhash icon;
	/** Image data for the role icon (if any) */
	std::string* image_data;

	/**
	 * @brief Construct a new role object
	 */
	role();

	/**
	 * @brief Destroy the role object
	 */
	virtual ~role();

	/**
	 * @brief Set the name of the role
	 * Maximum length: 100
	 * Minimum length: 1
	 * @param n Name to set
	 * @return role& reference to self
	 * @throw dpp::exception thrown if role length is less than 1 character
	 */
	role& set_name(const std::string& n);

	/**
	 * @brief Set the colour object
	 * 
	 * @param c Colour to set
	 * @note There is an americanised version of this method, role::set_color().
	 * @return role& reference to self
	 */
	role& set_colour(uint32_t c);

	/**
	 * @brief Set the color object
	 * 
	 * @param c Colour to set
	 * @note This is an alias of role::set_colour for American spelling.
	 * @return role& reference to self
	 */
	role& set_color(uint32_t c);

	/**
	 * @brief Set the flags object
	 * 
	 * @param f Flags to set
	 * @return role& reference to self
	 */
	role& set_flags(uint8_t f);

	/**
	 * @brief Set the integration id object
	 * 
	 * @param i Integration ID to set
	 * @return role& reference to self
	 */
	role& set_integration_id(snowflake i);

	/**
	 * @brief Set the bot id object
	 * 
	 * @param b Bot ID to set
	 * @return role& reference to self
	 */
	role& set_bot_id(snowflake b);

	/**
	 * @brief Set the guild id object
	 * 
	 * @param gid Guild ID to set
	 * @return role& reference to self
	 */
	role& set_guild_id(snowflake gid);

	/**
	 * @brief Fill this role from json.
	 * 
	 * @param guild_id the guild id to place in the json
	 * @param j The json data
	 * @return A reference to self
	 */
	role& fill_from_json(snowflake guild_id, nlohmann::json* j);

	/**
	 * @brief Build a json string from this object.
	 * 
	 * @param with_id true if the ID is to be included in the json text
	 * @return The json of the role
	 */
	std::string build_json(bool with_id = false) const;

	/**
	 * @brief Get the mention/ping for the role
	 * 
	 * @return std::string mention
	 */
	std::string get_mention() const;

	/**
	 * @brief Returns the role's icon if they have one, otherwise returns an empty string
	 *
	 * @param size The size of the icon in pixels. It can be any power of two between 16 and 4096. If not specified, the default sized icon is returned.
	 * @return std::string icon url or empty string
	 */
	std::string get_icon_url(uint16_t size = 0) const;

	/**
	 * @brief Load an image into the object as base64
	 * 
	 * @param image_blob Image binary data
	 * @param type Type of image
	 * @return emoji& Reference to self
	 */
	role& load_image(const std::string &image_blob, const image_type type);

	/**
	 * @brief True if the role is hoisted
	 * @return bool Role appears separated from others in the member list
	 */
	bool is_hoisted() const;
	/**
	 * @brief True if the role is mentionable
	 * @return bool Role is mentionable
	 */
	bool is_mentionable() const;
	/**
	 * @brief True if the role is managed (belongs to a bot or application)
	 * @return bool True if the role is managed (introduced for a bot or other application by Discord)
	 */
	bool is_managed() const;
	/**
	 * @brief True if has create instant invite permission
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the instant invite permission or is administrator.
	 */
	bool has_create_instant_invite() const;
	/**
	 * @brief True if has create instant invite permission
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the kick members permission or is administrator.
	 */
	bool has_kick_members() const;
	/**
	 * @brief True if has the ban members permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the ban members permission or is administrator.
	 */
	bool has_ban_members() const;
	/**
	 * @brief True if has the administrator permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the administrator permission or is administrator.
	 */
	bool has_administrator() const;
	/**
	 * @brief True if has the manage channels permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the manage channels permission or is administrator.
	 */
	bool has_manage_channels() const;
	/**
	 * @brief True if has the manage guild permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the manage guild permission or is administrator.
	 */
	bool has_manage_guild() const;
	/**
	 * @brief True if has the add reactions permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the add reactions permission or is administrator.
	 */
	bool has_add_reactions() const;
	/**
	 * @brief True if has the view audit log permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the view audit log permission or is administrator.
	 */
	bool has_view_audit_log() const;
	/**
	 * @brief True if has the priority speaker permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the priority speaker permission or is administrator.
	 */
	bool has_priority_speaker() const;
	/**
	 * @brief True if has the stream permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the stream permission or is administrator.
	 */
	bool has_stream() const;
	/**
	 * @brief True if has the view channel permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the view channel permission or is administrator.
	 */
	bool has_view_channel() const;
	/**
	 * @brief True if has the send messages permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the send messages permission or is administrator.
	 */
	bool has_send_messages() const;
	/**
	 * @brief True if has the send TTS messages permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the send TTS messages permission or is administrator.
	 */
	bool has_send_tts_messages() const;
	/**
	 * @brief True if has the manage messages permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the manage messages permission or is administrator.
	 */
	bool has_manage_messages() const;
	/**
	 * @brief True if has the embed links permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the embed links permission or is administrator.
	 */
	bool has_embed_links() const;
	/**
	 * @brief True if has the attach files permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the attach files permission or is administrator.
	 */
	bool has_attach_files() const;
	/**
	 * @brief True if has the read message history permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the read message history permission or is administrator.
	 */
	bool has_read_message_history() const;
	/**
	 * @brief True if has the mention \@everyone and \@here permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the mention \@everyone and \@here permission or is administrator.
	 */
	bool has_mention_everyone() const;
	/**
	 * @brief True if has the use external emojis permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the use external emojis permission or is administrator.
	 */
	bool has_use_external_emojis() const;
	/**
	 * @brief True if has the view guild insights permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the view guild insights permission or is administrator.
	 */
	bool has_view_guild_insights() const;
	/**
	 * @brief True if has the connect voice permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the connect voice permission or is administrator.
	 */
	bool has_connect() const;
	/**
	 * @brief True if has the speak permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the speak permission or is administrator.
	 */
	bool has_speak() const;
	/**
	 * @brief True if has the mute members permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the mute members permission or is administrator.
	 */
	bool has_mute_members() const;
	/**
	 * @brief True if has the deafen members permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the deafen members permission or is administrator.
	 */
	bool has_deafen_members() const;
	/**
	 * @brief True if has the move members permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the move members permission or is administrator.
	 */
	bool has_move_members() const;
	/** True if has use voice activity detection permission */
	bool has_use_vad() const;
	/**
	 * @brief True if has the change nickname permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the change nickname permission or is administrator.
	 */
	bool has_change_nickname() const;
	/**
	 * @brief True if has the manage nicknames permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the manage nicknames permission or is administrator.
	 */
	bool has_manage_nicknames() const;
	/**
	 * @brief True if has the manage roles permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the manage roles permission or is administrator.
	 */
	bool has_manage_roles() const;
	/**
	 * @brief True if has the manage webhooks permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the manage webhooks permission or is administrator.
	 */
	bool has_manage_webhooks() const;
	/**
	 * @brief True if has the manage emojis and stickers permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the manage emojis and stickers permission or is administrator.
	 */
	bool has_manage_emojis_and_stickers() const;
	/**
	 * @brief True if has the use application commands permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the use application commands permission or is administrator.
	 */
	bool has_use_application_commands() const;
	/**
	 * @brief True if has the request to speak permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the request to speak permission or is administrator.
	 */
	bool has_request_to_speak() const;
	/**
	 * @brief True if has the manage threads permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the manage threads permission or is administrator.
	 */
	bool has_manage_threads() const;
	/**
	 * @brief True if has the create public threads permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the create public threads permission or is administrator.
	 */
	bool has_create_public_threads() const;
	/**
	 * @brief True if has the create private threads permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the create private threads permission or is administrator.
	 */
	bool has_create_private_threads() const;
	/**
	 * @brief True if has the use external stickers permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the use external stickers permission or is administrator.
	 */
	bool has_use_external_stickers() const;
	/**
	 * @brief True if has the send messages in threads permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the send messages in threads permission or is administrator.
	 */
	bool has_send_messages_in_threads() const;
	/**
	 * @brief True if has the start embedded activities permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the start embedded activities permission or is administrator.
	 */
	bool has_start_embedded_activities() const;
	/**
	 * @brief True if has the manage events permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the manage events permission or is administrator.
	 */
	bool has_manage_events() const;
	/**
	 * @brief True if has the moderate users permission.
	 * @note Having the administrator permission causes this method to always return true
	 * Channel specific overrides may apply to permissions.
	 * @return bool True if user has the moderate users permission or is administrator.
	 */
	bool has_moderate_members() const;

	/**
	 * @brief Get guild members who have this role
	 * @note This method requires user/members cache to be active
	 * @return members_container List of members who have this role
	 */
	members_container get_members() const;
};

/** A group of roles */
typedef std::unordered_map<snowflake, role> role_map;

};

