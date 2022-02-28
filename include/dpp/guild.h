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
#include <dpp/snowflake.h>
#include <dpp/managed.h>
#include <dpp/utility.h>
#include <dpp/voicestate.h>
#include <string>
#include <unordered_map>

namespace dpp {

class channel;

/**
 * @brief Represents voice regions for guilds and channels.
 * @deprecated Deprecated in favour of per-channel regions.
 * Please use channel::rtc_region instead.
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
enum guild_flags : uint32_t {
	/** Large guild */
	g_large =				0b00000000000000000000000000000001,
	/** Unavailable guild (inaccessible due to an outage) */
	g_unavailable = 			0b00000000000000000000000000000010,
	/** Guild has widget enabled */
	g_widget_enabled =			0b00000000000000000000000000000100,
	/** Guild can  have an invite splash image */
	g_invite_splash =			0b00000000000000000000000000001000,
	/** Guild can have VIP regions */
	g_vip_regions =				0b00000000000000000000000000010000,
	/** Guild can have a vanity url */
	g_vanity_url =				0b00000000000000000000000000100000,
	/** Guild is verified */
	g_verified =				0b00000000000000000000000001000000,
	/** Guild is partnered */
	g_partnered =				0b00000000000000000000000010000000,
	/** Community features enabled */
	g_community =				0b00000000000000000000000100000000,
	/** Guild has commerce features enabled */
	g_commerce =				0b00000000000000000000001000000000,
	/** Guild has news features enabled */
	g_news =				0b00000000000000000000010000000000,
	/** Guild is discoverable in discovery */
	g_discoverable =			0b00000000000000000000100000000000,
	/** Guild is featureable */
	g_featureable =				0b00000000000000000001000000000000,
	/** Guild can have an animated icon (doesn't mean it actually has one though) */
	g_animated_icon =			0b00000000000000000010000000000000,
	/** Guild can have a banner image */
	g_banner =				0b00000000000000000100000000000000,
	/** Guild has a welcome screen */
	g_welcome_screen_enabled =		0b00000000000000001000000000000000,
	/** Guild has a member verification gate */
	g_member_verification_gate =		0b00000000000000010000000000000000,
	/** Guild has a preview */
	g_preview_enabled =			0b00000000000000100000000000000000,
	/** Guild join notifications are off */
	g_no_join_notifications =		0b00000000000001000000000000000000,
	/** Guild boost notifications are off */
	g_no_boost_notifications =		0b00000000000010000000000000000000,
	/** Guild has an actual animated icon (set by the icon hash starting with 'a_') */
	g_has_animated_icon =			0b00000000000100000000000000000000,
	/** Guild has an actual animated banner (set by the icon hash starting with 'a_') */
	g_has_animated_banner =			0b00000000001000000000000000000000,
	/** Guild setup tips are off */
	g_no_setup_tips =			0b00000000010000000000000000000000,
	/** "Wave to say hi" sticker prompt buttons are off */
	g_no_sticker_greeting =			0b00000000100000000000000000000000,
	/** guild has enabled monetization */
	g_monetization_enabled =		0b00000001000000000000000000000000,
	/** guild has increased custom sticker slots */
	g_more_stickers =			0b00000010000000000000000000000000,
	/** guild has access to create private threads */
	g_private_threads =			0b00000100000000000000000000000000,
	/** guild is able to set role icons */
	g_role_icons =				0b00001000000000000000000000000000,
	/** guild has access to the seven day archive time for threads */
	g_seven_day_thread_archive =		0b00010000000000000000000000000000,
	/** guild has access to the three day archive time for threads */
	g_three_day_thread_archive =		0b00100000000000000000000000000000,
	/** guild has enabled ticketed events */
	g_ticketed_events =			0b01000000000000000000000000000000,
	/** guild can have channel banners */
	g_channel_banners =			0b10000000000000000000000000000000,
};

/**
 * @brief Various flags that can be used to indicate the status of a guild member
 */
enum guild_member_flags : uint8_t {
	/** Member deafened in voice channels */
	gm_deaf =		0b00001,
	/** Member muted in voice channels */
	gm_mute =		0b00010,
	/** Member pending verification by membership screening */
	gm_pending =		0b00100,
	/** Member has animated guild-specific avatar */
	gm_animated_avatar = 	0b01000,
};

/**
 * @brief Represents dpp::user membership upon a dpp::guild.
 * This contains the user's nickname, guild roles, and any other guild-specific flags.
 */
class DPP_EXPORT guild_member {
public:
	/** Nickname, or empty string if they don't have a nickname on this guild */
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
	/** User avatar (per-server avatar is a nitro only feature) */
	utility::iconhash avatar;
	/** timestamp of when the time out will be removed; until then, they cannot interact with the guild */
	time_t communication_disabled_until;

	/** Default constructor */
	guild_member();

	/** Fill this object from a json object.
	 * @param j The json object to get data from
	 * @param g_id The guild id to associate the member with
	 * @param u_id The user id to associate the member with
	 * @return Reference to self for call chaining
	 */
	guild_member& fill_from_json(nlohmann::json* j, snowflake g_id, snowflake u_id);

	/**
	 * @brief Build json string for the member object
	 * 
	 * @return std::string json string
	 */
	std::string build_json() const;

	/**
	 * @brief Returns true if the user is in time-out (communication disabled)
	 * 
	 * @return true user is in time-out
	 * @return false user is not in time-out
	 */
	bool is_communication_disabled() const;

	/**
	 * @brief Returns true if the user is deafened
	 * 
	 * @return true user is deafened
	 * @return false user is not deafened
	 */
	bool is_deaf() const;

	/**
	 * @brief Returns true if the user is muted
	 * 
	 * @return true user muted
	 * @return false user not muted
	 */
	bool is_muted() const;

	/**
	 * @brief Returns true if pending verification by membership screening
	 * 
	 * @return true user has completed membership screening
	 * @return false user has not completed membership screening
	 */
	bool is_pending() const;

	/**
	 * @brief Returns true if the user's per-guild custom avatar is animated
	 * 
	 * @return true user's custom avatar is animated
	 * @return false user's custom avatar is not animated
	 */
	bool has_animated_guild_avatar() const;

	/**
	 * @brief Returns the members per guild avatar if they have one, otherwise returns an empty string
	 *
	 * @note per-server avatar is a nitro only feature so it might be not set. If you need the real user avatar, use user::get_avatar_url.
	 * 
	 * @param size The size of the avatar in pixels. It can be any power of two between 16 and 4096. If not specified, the default sized avatar is returned.
	 * @return std::string avatar url or empty string
	 */
	std::string get_avatar_url(uint16_t size = 0) const;

	/**
	 * @brief Set the nickname 
	 * 
	 * @param nick Nickname to set
	 * 
	 * @return guild_member& reference to self
	 */
	guild_member& set_nickname(const std::string& nick);

	/**
	 * @brief Set whether the user is muted in voice channels
	 *
	 * @param is_muted value to set, true if mute in voice channels
	 * 
	 * @return guild_member& reference to self 
	 */
	guild_member& set_mute(const bool is_muted);

	/**
	 * @brief Set whether the user is deafened in voice channels
	 *
	 * @param is_deafened value to set, true if deaf in voice channels
	 * 
	 * @return guild_member& reference to self 
	 */
	guild_member& set_deaf(const bool is_deafened);

	/**
	 * @brief Set communication_disabled_until
	 *
	 * @param timestamp timestamp until communication is disabled
	 *
	 * @return guild_member& reference to self
	 */
	guild_member& set_communication_disabled_until(const time_t timestamp);

	/**
	 * @brief Return a ping/mention for the user by nickname
	 * 
	 * @return std::string mention
	 */
	std::string get_mention() const;
};

/**
 * @brief Defines a channel on a server's welcome screen
 */
struct welcome_channel_t {
	/// the channel's id
	snowflake channel_id = 0;

	/// the description shown for the channel
	std::string description;

	/// the emoji id, if the emoji is custom
	snowflake emoji_id = 0;

	/// the emoji name if custom, the unicode character if standard, or null if no emoji is set
	std::string emoji_name;
};


/**
 * @brief Defines a server's welcome screen
 */
struct welcome_screen_t {
	/// the server description shown in the welcome screen
	std::string description;
	/// the channels shown in the welcome screen, up to 5
	std::vector<welcome_channel_t> welcome_channels;
};

/**
 * @brief Guild NSFW level.
 * Used to represent just how naughty this guild is. Naughty  guild, go sit in the corner.
 * @note This is set by Discord, and cannot be set by any bot or user on the guild.
 */
enum guild_nsfw_level_t : uint8_t {
	/// Default setting, not configured
	nsfw_default		=	0,
	/// Explicit content may be in this guild
	nsfw_explicit		=	1,
	/// Safe for work content only
	nsfw_safe		=	2,
	/// Age restricted, 18+
	nsfw_age_restricted	=	3
};

/**
 * @brief explicit content filter level.
 * This is set by a guild admin, but can be forced to a setting if the server is verified,
 * partnered, official etc.
 */
enum guild_explicit_content_t : uint8_t {
	/// media content will not be scanned
	expl_disabled =			0,
	/// media content sent by members without roles will be scanned
	expl_members_without_roles =	1,
	/// media content sent by all members will be scanned
	expl_all_members =		2
};

/**
 * @brief MFA level for server. If set to elevated all moderators need MFA to perform specific
 * actions such as kick or ban.
 */
enum mfa_level_t : uint8_t {
	/// MFA not elevated
	mfa_none = 0,
	/// MFA elevated
	mfa_elevated = 1
};

/**
 * @brief Guild verification level
 */
enum verification_level_t : uint8_t {
	/// unrestricted
	ver_none =	0,
	/// must have verified email on account
	ver_low	= 	1,
	/// must be registered on Discord for longer than 5 minutes
	ver_medium =	2,
	/// must be a member of the server for longer than 10 minutes
	ver_high =	3,
	/// must have a verified phone number
	ver_very_high =	4,
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

	/**
	 * @brief Vanity url code for verified or partnered servers and boost level 3
	 * @note This field cannot be set from the API. Attempts to change this value will be
	 * silently ignored even if the correct number of boosts or verified/partnered status exist.
	 * See: https://github.com/discord/discord-api-docs/issues/519
	 */
	std::string vanity_url_code;

	/** Guild icon hash */
	utility::iconhash icon;

	/** Guild splash hash */
	utility::iconhash splash;

	/** Guild discovery splash hash */
	utility::iconhash discovery_splash;

	/** Snowflake id of guild owner */
	snowflake owner_id;

	/** Snowflake ID of AFK voice channel or 0 */
	snowflake afk_channel_id;

	/** Voice AFK timeout before moving users to AFK channel */
	uint8_t afk_timeout;

	/** Snowflake ID of widget channel, or 0 */
	snowflake widget_channel_id;

	/** Verification level of server */
	verification_level_t verification_level;

	/** Setting for how notifications are to be delivered to users */
	uint8_t default_message_notifications;

	/** Whether or not explicit content filtering is enable and what setting it is */
	guild_explicit_content_t explicit_content_filter;

	/** If multi factor authentication is required for moderators or not */
	mfa_level_t mfa_level;

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

	/** Welcome screen
	 */
	welcome_screen_t welcome_screen;

	/**
	 * @brief the maximum number of presences for the guild.
	 * @note Generally Discord always fills this with 0, apart from for the largest of guilds
	 */
	uint32_t max_presences;

	/**
	 * @brief the maximum number of members for the guild
	 */
	uint32_t max_members;

	/**
	 * @brief Guild NSFW level
	 */
	guild_nsfw_level_t nsfw_level;

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
	 * @return JSON string
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

    /**
	 * @brief Get the banner url of the guild if it have one, otherwise returns an empty string
	 *
	 * @param size The size of the banner in pixels. It can be any power of two between 16 and 4096. if not specified, the default sized banner is returned.
	 * @return std::string banner url or empty string
	 */
    std::string get_banner_url(uint16_t size = 0) const;

    /**
	 * @brief Get the discovery splash url of the guild if it have one, otherwise returns an empty string
	 *
	 * @param size The size of the discovery splash in pixels. It can be any power of two between 16 and 4096. if not specified, the default sized discovery splash is returned.
	 * @return std::string discovery splash url or empty string
	 */
    std::string get_discovery_splash_url(uint16_t size = 0) const;

    /**
	 * @brief Get the icon url of the guild if it have one, otherwise returns an empty string
	 *
	 * @param size The size of the icon in pixels. It can be any power of two between 16 and 4096. if not specified, the default sized icon is returned.
	 * @return std::string icon url or empty string
	 */
    std::string get_icon_url(uint16_t size = 0) const;

    /**
	 * @brief Get the splash url of the guild if it have one, otherwise returns an empty string
	 *
	 * @param size The size of the splash in pixels. It can be any power of two between 16 and 4096. if not specified, the default sized splash is returned.
	 * @return std::string splash url or empty string
	 */
    std::string get_splash_url(uint16_t size = 0) const;

	/**
	 * @brief Set the name of the guild in the object
	 * Min length: 2, Max length: 100 (not including leading/trailing spaces)
	 * @param n Guild name
	 * @return guild& reference to self
	 * @throw dpp::length_exception if guild name is too short
	 */
	guild& set_name(const std::string& n);

	/**
	 * @brief Is a large server (>250 users)
	 * @return bool is a large guild
	 */
	bool is_large() const;

	/**
	 * @brief Is unavailable due to outage (most other fields will be blank or outdated
	 * @return bool is unavailable
	 */
	bool is_unavailable() const;

	/**
	 * @brief Widget is enabled for this server
	 * @return bool widget enabled
	 */
	bool widget_enabled() const;

	/**
	 * @brief Guild has an invite splash
	 * @return bool has an invite splash
	 */
	bool has_invite_splash() const;

	/**
	 * @brief Guild has VIP voice regions
	 * @return bool has vip regions
	 */
	bool has_vip_regions() const;

	/**
	 * @brief Guild can have a vanity url
	 * @return bool can have vanity url
	 */
	bool has_vanity_url() const;

	/**
	 * @brief Guild is a verified server
	 * @return bool is verified
	 */
	bool is_verified() const;

	/**
	 * @brief Guild is a discord partnered server
	 * @return bool is discord partnered
	 */
	bool is_partnered() const;

	/**
	 * @brief Has enabled community
	 * @return bool has enabled community
	 */
	bool is_community() const;

	/**
	 * @brief Guild has commerce channels
	 * @return bool has commerce guilds
	 */
	bool has_commerce() const;

	/**
	 * @brief Guild has news channels
	 * @return bool has news channels
	 */
	bool has_news() const;

	/**
	 * @brief Guild is discoverable
	 * @return bool is discoverable
	 */
	bool is_discoverable() const;

	/**
	 * @brief Guild is featurable
	 * @return bool is featurable
	 */
	bool is_featureable() const;

	/**
	 * @brief Guild can have an animated icon
	 * @return bool can have animated icon
	 */
	bool has_animated_icon() const;

	/**
	 * @brief Guild has a banner image
	 * @return bool has banner image
	 */
	bool has_banner() const;

	/**
	 * @brief Guild has enabled the welcome screen
	 * @return bool enabled welcome screen
	 */
	bool is_welcome_screen_enabled() const;

	/**
	 * @brief Guild has membership screening
	 * @return bool has membership screening
	 */
	bool has_member_verification_gate() const;

	/**
	 * @brief Guild has preview enabled
	 * @return bool has preview
	 */
	bool is_preview_enabled() const;

	/**
	 * @brief Guild icon is actually an animated gif
	 * @return bool is animated gif
	 */
	bool has_animated_icon_hash() const;

	/**
	 * @brief Guild banner is animated gif
	 * @return bool is animated gif
	 */
	bool has_animated_banner_icon_hash() const;


	/**
	 * @brief guild has access to monetization features
	 * @return bool 
	 */
	bool has_monetization_enabled() const;

	/**
	 * @brief guild has increased custom sticker slots
	 * @return bool has more stickers
	 */
	bool has_more_stickers() const;

	/**
	 * @brief guild has access to create private threads
	 * @return bool has private threads
	 */
	bool has_private_threads() const;

	/**
	 * @brief guild is able to set role icons
	 * @return bool has role icons
	 */
	bool has_role_icons() const;

	/**
	 * @brief guild has access to the seven day archive time for threads 
	 * @return bool has seven day thread archive
	 */
	bool has_seven_day_thread_archive() const;

	/**
	 * @brief guild has access to the three day archive time for threads
	 * @return bool has three day thread archive
	 */
	bool has_three_day_thread_archive() const;

	/**
	 * @brief guild has enabled ticketed events
	 * @return bool has ticketed events
	 */
	bool has_ticketed_events() const;

	/**
	 * @brief guild has access to channel banners feature
	 * @return bool has channel banners
	 */
	bool has_channel_banners() const;
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

/**
 * @brief Get the guild_member from cache of given IDs
 *
 * @param guild_id ID of the guild to find guild_member for
 * @param user_id ID of the user to find guild_member for
 *
 * @throw dpp::cache_exception if the guild or guild_member is not found in the cache
 * @return guild_member the cached object, if found
 */
guild_member DPP_EXPORT find_guild_member(const snowflake guild_id, const snowflake user_id);

};
