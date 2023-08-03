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
#include <dpp/json.h>
#include <cstdint>
#include <type_traits>

namespace dpp {

/**
 * @brief Represents the various discord permissions
 */
enum permissions : uint64_t {
	p_create_instant_invite = 0x00000000001,    //!< allows creation of instant invites
	p_kick_members = 0x00000000002,    //!< allows kicking members
	p_ban_members = 0x00000000004,    //!< allows banning members
	p_administrator = 0x00000000008,    //!< allows all permissions and bypasses channel permission overwrites
	p_manage_channels = 0x00000000010,    //!< allows management and editing of channels
	p_manage_guild = 0x00000000020,    //!< allows management and editing of the guild
	p_add_reactions = 0x00000000040,    //!< allows for the addition of reactions to messages
	p_view_audit_log = 0x00000000080,    //!< allows for viewing of audit logs
	p_priority_speaker = 0x00000000100,    //!< allows for using priority speaker in a voice channel
	p_stream = 0x00000000200,    //!< allows the user to go live
	p_view_channel = 0x00000000400,    //!< allows guild members to view a channel, which includes reading messages in text channels and joining voice channels
	p_send_messages = 0x00000000800,    //!< allows for sending messages in a channel
	p_send_tts_messages = 0x00000001000,    //!< allows for sending of /tts messages
	p_manage_messages = 0x00000002000,    //!< allows for deletion of other users messages
	p_embed_links = 0x00000004000,    //!< links sent by users with this permission will be auto-embedded
	p_attach_files = 0x00000008000,    //!< allows for uploading images and files
	p_read_message_history = 0x00000010000,    //!< allows for reading of message history
	p_mention_everyone = 0x00000020000,    //!< allows for using the everyone and the here tag to notify users in a channel
	p_use_external_emojis = 0x00000040000,    //!< allows the usage of custom emojis from other servers
	p_view_guild_insights = 0x00000080000,    //!< allows for viewing guild insights
	p_connect = 0x00000100000,    //!< allows for joining of a voice channel
	p_speak = 0x00000200000,    //!< allows for speaking in a voice channel
	p_mute_members = 0x00000400000,    //!< allows for muting members in a voice channel
	p_deafen_members = 0x00000800000,    //!< allows for deafening of members in a voice channel
	p_move_members = 0x00001000000,    //!< allows for moving of members between voice channels
	p_use_vad = 0x00002000000,    //!< allows for using voice-activity-detection in a voice channel
	p_change_nickname = 0x00004000000,    //!< allows for modification of own nickname
	p_manage_nicknames = 0x00008000000,    //!< allows for modification of other users nicknames
	p_manage_roles = 0x00010000000,    //!< allows management and editing of roles
	p_manage_webhooks = 0x00020000000,    //!< allows management and editing of webhooks
	p_manage_emojis_and_stickers = 0x00040000000,    //!< allows management and editing of emojis and stickers
	p_use_application_commands = 0x00080000000,    //!< allows members to use application commands, including slash commands and context menus
	p_request_to_speak = 0x00100000000,    //!< allows for requesting to speak in stage channels. (Discord: This permission is under active development and may be changed or removed.)
	p_manage_events = 0x00200000000,    //!< allows for management (creation, updating, deleting, starting) of scheduled events
	p_manage_threads = 0x00400000000,    //!< allows for deleting and archiving threads, and viewing all private threads
	p_create_public_threads = 0x00800000000,    //!< allows for creating public and announcement threads
	p_create_private_threads = 0x01000000000,    //!< allows for creating private threads
	p_use_external_stickers = 0x02000000000,    //!< allows the usage of custom stickers from other servers
	p_send_messages_in_threads = 0x04000000000,    //!< allows for sending messages in threads
	p_use_embedded_activities = 0x08000000000,    //!< allows for using activities (applications with the EMBEDDED flag) in a voice channel
	p_moderate_members = 0x10000000000,    //!< allows for timing out users to prevent them from sending or reacting to messages in chat and threads, and from speaking in voice and stage channels
	p_view_creator_monetization_analytics = 0x20000000000,	//!< allows for viewing role subscription insights
	p_use_soundboard = 0x40000000000, //!< allows for using soundboard in a voice channel
	p_use_external_sounds = 0x0000200000000000, //!< allows the usage of custom soundboard sounds from other servers
	p_send_voice_messages = 0x0000400000000000, //!< allows sending voice messages
};

/**
 * @brief Represents the various discord permissions
 * @deprecated Use dpp::permissions instead.
 */
using role_permissions = permissions;

/**
 * @brief Represents a permission bitmask (refer to enum dpp::permissions) which are held in an uint64_t
 */
class DPP_EXPORT permission {
protected:
	/**
	 * @brief The permission bitmask value
	 */
	uint64_t value{0};

public:
	/**
	 * @brief Default constructor, initializes permission to 0
	 */
	constexpr permission() = default;

	/**
	 * @brief Bitmask constructor, initializes permission to the argument
	 * @param value The bitmask to initialize the permission to
	 */
	constexpr permission(uint64_t value) noexcept : value{value} {}

	/**
	 * @brief For acting like an integer
	 * @return The permission bitmask value
	 */
	constexpr operator uint64_t() const noexcept {
		return value;
	}

	/**
	 * @brief For acting like an integer
	 * @return A reference to the permission bitmask value
	 */
	constexpr operator uint64_t &() noexcept {
		return value;
	}

	/**
	 * @brief For building json
	 * @return The permission bitmask value as a string
	 */
	operator nlohmann::json() const;

	/**
	 * @brief Check for permission flags set. It uses the Bitwise AND operator
	 * @tparam T one or more uint64_t permission bits
	 * @param values The permissions (from dpp::permissions) to check for
	 *
	 * **Example:**
	 *
	 * ```cpp
	 * bool is_mod = permission.has(dpp::p_kick_members, dpp::p_ban_members);
	 * // Returns true if the permission bitmask contains p_kick_members and p_ban_members
	 * ```
	 *
	 * @return bool True if it has all the given permissions
	 */
	template <typename... T>
	constexpr bool has(T... values) const noexcept {
		return (value & (0 | ... | values)) == (0 | ... | values);
	}

	/**
	 * @brief Check for permission flags set. It uses the Bitwise AND operator
	 * @tparam T one or more uint64_t permission bits
	 * @param values The permissions (from dpp::permissions) to check for
	 *
	 * **Example:**
	 *
	 * ```cpp
	 * bool is_mod = permission.has_any(dpp::p_administrator, dpp::p_ban_members);
	 * // Returns true if the permission bitmask contains p_administrator or p_ban_members
	 * ```
	 *
	 * @return bool True if it has any the given permissions
	 */
	template <typename... T>
	constexpr bool has_any(T... values) const noexcept {
		return (value & (0 | ... | values)) != 0;
	}

	/**
	 * @brief Add a permission with the Bitwise OR operation
	 * @tparam T one or more uint64_t permission bits
	 * @param values The permissions (from dpp::permissions) to add
	 *
	 * **Example:**
	 *
	 * ```cpp
	 * permission.add(dpp::p_view_channel, dpp::p_send_messages);
	 * // Adds p_view_channel and p_send_messages to the permission bitmask
	 * ```
	 *
	 * @return permission& reference to self for chaining
	 */
	template <typename... T>
	std::enable_if_t<(std::is_convertible_v<T, uint64_t> && ...), permission&>
	constexpr add(T... values) noexcept {
		value |= (0 | ... | values);
		return *this;
	}

	/**
	 * @brief Assign a permission. This will reset the bitmask to the new value.
	 * @tparam T one or more uint64_t permission bits
	 * @param values The permissions (from dpp::permissions) to set
	 *
	 * **Example:**
	 *
	 * ```cpp
	 * permission.set(dpp::p_view_channel, dpp::p_send_messages);
	 * ```
	 *
	 * @return permission& reference to self for chaining
	 */
	template <typename... T>
	std::enable_if_t<(std::is_convertible_v<T, uint64_t> && ...), permission&>
	constexpr set(T... values) noexcept {
		value = (0 | ... | values);
		return *this;
	}

	/**
	 * @brief Remove a permission with the Bitwise NOT operation
	 * @tparam T one or more uint64_t permission bits
	 * @param values The permissions (from dpp::permissions) to remove
	 *
	 * **Example:**
	 *
	 * ```cpp
	 * permission.remove(dpp::p_view_channel, dpp::p_send_messages);
	 * // Removes p_view_channel and p_send_messages permission
	 * ```
	 *
	 * @return permission& reference to self for chaining
	 */
	template <typename... T>
	std::enable_if_t<(std::is_convertible_v<T, uint64_t> && ...), permission&>
	constexpr remove(T... values) noexcept {
		value &= ~(0 | ... | values);
		return *this;
	}
};

}
