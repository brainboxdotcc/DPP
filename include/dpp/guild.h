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
#include <mutex>
#include <string>
#include <unordered_map>
#include <map>
#include <dpp/voicestate.h>

namespace dpp {

/**
 * @brief Represents voice regions for guilds and channels.
 * @note Largely deprecated in favour of per-channel regions.
 */
enum region : uint8_t {
	r_brazil,		//!< Brazil
	r_central_europe,	//!< Central Europe
	r_hong_kong,		//!< Hong Kong
	r_india,		//!< India
	r_japan,		//!< Japan
	r_russia,		//!< Russia
	r_singapore,		//!< Singapore
	r_south_africa,		//!< South Africa
	r_sydney,		//!< Sydney
	r_us_central,		//!< US Central
	r_us_east,		//!< US East Coast
	r_us_south,		//!< US South
	r_us_west,		//!< US West Coast
	r_western_europe	//!< Western Europe
};

/**
 * @brief The various flags that represent the status of a dpp::guild object
 */
enum guild_flags {
	/** Large guild */
	g_large =				0b000000000000000000001,
	/** Unavailable guild (inaccessible due to an outage) */
	g_unavailable = 			0b000000000000000000010,
	/** Guild has widget enabled */
	g_widget_enabled =			0b000000000000000000100,
	/** Guild can  have an invite splash image */
	g_invite_splash =			0b000000000000000001000,
	/** Guild can have VIP regions */
	g_vip_regions =				0b000000000000000010000,
	/** Guild can have a vanity url */
	g_vanity_url =				0b000000000000000100000,
	/** Guild is verified */
	g_verified =				0b000000000000001000000,
	/** Guild is partnered */
	g_partnered =				0b000000000000010000000,
	/** Community features enabled */
	g_community =				0b000000000000100000000,
	/** Guild has commerce features enabled */
	g_commerce =				0b000000000001000000000,
	/** Guild has news features enabled */
	g_news =				0b000000000010000000000,
	/** Guild is discoverable in discovery */
	g_discoverable =			0b000000000100000000000,
	/** Guild is featureable */
	g_featureable =				0b000000001000000000000,
	/** Guild can have an animated icon (doesn't mean it actually has one though) */
	g_animated_icon =			0b000000010000000000000,
	/** Guild can have a banner image */
	g_banner =				0b000000100000000000000,
	/** Guild has a welcome screen */
	g_welcome_screen_enabled =		0b000001000000000000000,
	/** Guild has a member verification gate */
	g_member_verification_gate =		0b000010000000000000000,
	/** Guild has a preview */
	g_preview_enabled =			0b000100000000000000000,
	/** Guild join notifications are off */
	g_no_join_notifications =		0b001000000000000000000,
	/** Guild boost notifications are off */
	g_no_boost_notifications =		0b010000000000000000000,
	/** Guild has an actual animated icon (set by the icon hash starting with 'a_') */
	g_has_animated_icon =			0b100000000000000000000
};

/**
 * @brief Various flags that can be used to indicate the status of a guild member
 */
enum guild_member_flags {
	/** Member deafened */
	gm_deaf =		0b00001,
	/** Member muted */
	gm_mute =		0b00010,
	/** Member pending verification by membership screening */
	gm_pending =		0b00100
};

/**
 * @brief Represents dpp::user membership upon a dpp::guild
 */
class DPP_EXPORT guild_member {
public:
	/** Nickname, or nullptr if they don't have a nickname on this guild */
	std::string nickname;
	/** Guild id */
	snowflake guild_id;
	/** User id */
	snowflake user_id;
	/** List of roles this user has on this guild */
	std::vector<snowflake> roles;
	/** Date and time the user joined the guild */
	time_t joined_at;
	/** Boosting since */
	time_t premium_since;
	/** A set of flags built from the bitmask defined by dpp::guild_member_flags */
	uint8_t flags;

	/** Default constructor */
	guild_member();

	/** Fill this object from a json object.
	 * @param j The json object to get data from
	 * @param g_id The guild id to associate the member with
	 * @param u_id The user id to associate the member with
	 */
	guild_member& fill_from_json(nlohmann::json* j, snowflake g_id, snowflake u_id);

	/** Build json string for the member object */
	std::string build_json() const;

	/** Returns true if the user is deafened */
	bool is_deaf() const;

	/** Returns true if the user is muted */
	bool is_muted() const;

	/** Returns true if pending verification by membership screening */
	bool is_pending() const;
	
};

/** @brief Guild members container
 */
typedef std::unordered_map<snowflake, guild_member> members_container;

/**
 * @brief Represents a guild on Discord (AKA a server)
 */
class DPP_EXPORT guild : public managed {
public:
	/** Shard ID of the guild */
	uint16_t shard_id;

	/** Flags bitmask as defined by values within dpp::guild_flags */
	uint32_t flags;

	/** Guild name */
	std::string name;

	/** Server description for communities */
	std::string description;

	/** Vanity url code for verified or partnered servers and boost level 3 */
	std::string vanity_url_code;

	/** Guild icon hash */
	utility::iconhash icon;

	/** Guild splash hash */
	utility::iconhash splash;

	/** Guild discovery splash hash */
	utility::iconhash discovery_splash;

	/** Snowflake id of guild owner */
	snowflake owner_id;

	/** Guild voice region */
	region voice_region;

	/** Snowflake ID of AFK voice channel or 0 */
	snowflake afk_channel_id;

	/** Voice AFK timeout before moving users to AFK channel */
	uint8_t afk_timeout;

	/** Snowflake ID of widget channel, or 0 */
	snowflake widget_channel_id;

	/** Verification level of server */
	uint8_t verification_level;

	/** Setting for how notifications are to be delivered to users */
	uint8_t default_message_notifications;

	/** Whether or not explicit content filtering is enable and what setting it is */
	uint8_t explicit_content_filter;

	/** If multi factor authentication is required for moderators or not */
	uint8_t mfa_level;

	/** ID of creating application, if any, or 0 */
	snowflake application_id;

	/** ID of system channel where discord update messages are sent */
	snowflake system_channel_id;

	/** ID of rules channel for communities */
	snowflake rules_channel_id;

	/** Approximate member count. May be sent as zero */
	uint32_t member_count;

	/** Server banner hash */
	utility::iconhash banner;

	/** Boost level */
	uint8_t premium_tier;

	/** Number of boosters */
	uint16_t premium_subscription_count;

	/** Public updates channel id or 0 */
	snowflake public_updates_channel_id;

	/** Maximum users in a video channel, or 0 */
	uint16_t max_video_channel_users;

	/** Roles defined on this server */
	std::vector<snowflake> roles;

	/** List of channels on this server */
	std::vector<snowflake> channels;

	/** List of threads on this server */
	std::vector<snowflake> threads;

	/** List of guild members. Note that when you first receive the
	 * guild create event, this may be empty or near empty.
	 * This depends upon your dpp::intents and the size of your bot.
	 * It will be filled by guild member chunk requests.
	 */
	members_container members;

	/** List of members in voice channels in the guild.
	 */
	std::map<snowflake, voicestate> voice_members;

        /** List of emojis
	 */
	std::vector<snowflake> emojis;

	/** Default constructor, zeroes all values */
	guild();

	/**
	 * @brief Destroy the guild object
	 */
	virtual ~guild() = default;

	/** Read class values from json object
	 * @param shard originating shard
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	guild& fill_from_json(class discord_client* shard, nlohmann::json* j);

	/** Build a JSON string from this object.
	 * @param with_id True if an ID is to be included in the JSON
	 */
	std::string build_json(bool with_id = false) const;

	/**
	 * @brief Get the base permissions for a member on this guild,
	 * before permission overwrites are applied.
	 *
	 * @param member member to get permissions for
	 * @return uint64_t permissions bitmask
	 */
	uint64_t base_permissions(const class user* member) const;

	/**
	 * @brief Get the permission overwrites for a member
	 * merged into a bitmask.
	 *
	 * @param base_permissions base permissions before overwrites,
	 * from channel::base_permissions
	 * @param member Member to fetch permissions for
	 * @param channel Channel to fetch permissions against
	 * @return uint64_t Merged permissions bitmask of overwrites.
	 */
	uint64_t permission_overwrites(const uint64_t base_permissions, const user*  member, const channel* channel) const;

	/**
	 * @brief Rehash members map
	 */
	void rehash_members();

	/**
	 * @brief Connect to a voice channel another guild member is in
	 *
	 * @param user_id User id to join
	 * @param self_mute True if the bot should mute itself
	 * @param self_deaf True if the bot should deafen itself
	 * @return True if the user specified is in a vc, false if they aren't
	 */
	bool connect_member_voice(snowflake user_id, bool self_mute = false, bool self_deaf = false);

	/** Is a large server (>250 users) */
	bool is_large() const;

	/** Is unavailable due to outage (most other fields will be blank or outdated */
	bool is_unavailable() const;

	/** Widget is enabled for this server */
	bool widget_enabled() const;

	/** Guild has an invite splash */
	bool has_invite_splash() const;

	/** Guild has VIP regions */
	bool has_vip_regions() const;

	/** Guild can have a vanity url */
	bool has_vanity_url() const;

	/** Guild is a verified server */
	bool is_verified() const;

	/** Guild is a discord partner server */
	bool is_partnered() const;

	/** Guild has enabled community */
	bool is_community() const;

	/** Guild has enabled commerce channels */
	bool has_commerce() const;

	/** Guild has news channels */
	bool has_news() const;

	/** Guild is discoverable */
	bool is_discoverable() const;

	/** Guild is featureable */
	bool is_featureable() const;

	/** Guild is allowed an animated icon */
	bool has_animated_icon() const;

	/** Guild has a banner image */
	bool has_banner() const;

	/** Guild has enabled welcome screen */
	bool is_welcome_screen_enabled() const;

	/** Guild has enabled membership screening */
	bool has_member_verification_gate() const;

	/** Guild has preview enabled */
	bool is_preview_enabled() const;

	/** Server icon is actually an animated gif */
	bool has_animated_icon_hash() const;

};

/** A container of guilds */
typedef std::unordered_map<snowflake, guild> guild_map;

/**
 * @brief Represents a guild widget, simple web widget of member list
 */
class DPP_EXPORT guild_widget {
public:
	/**
	 * @brief True if enabled
	 */
	bool enabled;
	/**
	 * @brief Channel widget points to
	 */
	snowflake channel_id;

	/**
	 * @brief Construct a new guild widget object
	 */
	guild_widget();

	/**
	 * @brief Build a guild widget from json
	 *
	 * @param j json to build from
	 * @return guild_widget& reference to self
	 */
	guild_widget& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build json for a guild widget
	 *
	 * @return std::string guild widget stringified json
	 */
	std::string build_json() const;
};

/**
 * @brief helper function to deserialize a guild_member from json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param gm guild_member to be deserialized
 */
void from_json(const nlohmann::json& j, guild_member& gm);

/** A container of guild members */
typedef std::unordered_map<snowflake, guild_member> guild_member_map;


};
