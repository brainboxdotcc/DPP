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
#include <dpp/nlohmann/json_fwd.hpp>
#include <dpp/permissions.h>
#include <dpp/guild.h>
#include <dpp/json_interface.h>

namespace dpp {

/** Various flags related to dpp::role */
enum role_flags : uint8_t {
	r_hoist =		0b00000001, //!< Hoisted role
	r_managed =		0b00000010, //!< Managed role (introduced by a bot or application)
	r_mentionable =		0b00000100, //!< Mentionable with a ping
	r_premium_subscriber =	0b00001000, //!< This is set for the role given to nitro 
};

/**
 * @brief Represents a role within a dpp::guild.
 * Roles are combined via logical OR of the permission bitmasks, then channel-specific overrides
 * can be applied on top, deny types apply a logic NOT to the bit mask, and allows apply a logical OR.
 * @note Every guild has at least one role, called the 'everyone' role, which always has the same role
 * ID as the guild's ID. This is the base permission set applied to all users where no other role or override
 * applies, and is the starting value of the bit mask looped through to calculate channel permissions.
 */
class DPP_EXPORT role : public managed, public json_interface<role>  {
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
	/** Role permissions bitmask values from dpp::permissions */
	permission permissions;
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
	 * @brief Set the colour
	 * 
	 * @param c Colour to set
	 * @note There is an americanised version of this method, role::set_color().
	 * @return role& reference to self
	 */
	role& set_colour(uint32_t c);

	/**
	 * @brief Set the color
	 * 
	 * @param c Colour to set
	 * @note This is an alias of role::set_colour for American spelling.
	 * @return role& reference to self
	 */
	role& set_color(uint32_t c);

	/**
	 * @brief Set the flags
	 * 
	 * @param f Flags to set
	 * @return role& reference to self
	 */
	role& set_flags(uint8_t f);

	/**
	 * @brief Set the integration id
	 * 
	 * @param i Integration ID to set
	 * @return role& reference to self
	 */
	role& set_integration_id(snowflake i);

	/**
	 * @brief Set the bot id
	 * 
	 * @param b Bot ID to set
	 * @return role& reference to self
	 */
	role& set_bot_id(snowflake b);

	/**
	 * @brief Set the guild id
	 * 
	 * @param gid Guild ID to set
	 * @return role& reference to self
	 */
	role& set_guild_id(snowflake gid);

	/**
	 * @brief Fill this role from json.
	 * 
	 * @param j The json data
	 * @return A reference to self
	 */
	role& fill_from_json(nlohmann::json* j);

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
	virtual std::string build_json(bool with_id = false) const;

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
	bool has_use_embedded_activities() const;
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

