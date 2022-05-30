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
#include <dpp/nlohmann/json_fwd.hpp>
#include <dpp/permissions.h>
#include <dpp/json_interface.h>
#include <unordered_map>

namespace dpp {

/** @brief Flag integers as received from and sent to discord */
enum channel_type : uint8_t {
	CHANNEL_TEXT		= 0,	//!< a text channel within a server
	DM			= 1,	//!< a direct message between users
	CHANNEL_VOICE		= 2,	//!< a voice channel within a server
	/**
	 * @brief a direct message between multiple users
	 * @deprecated this channel type was intended to be used with the now deprecated GameBridge SDK. Existing group dms with bots will continue to function, but newly created channels will be unusable
	 */
	GROUP_DM		= 3,
	CHANNEL_CATEGORY	= 4,	//!< an organizational category that contains up to 50 channels
	CHANNEL_NEWS		= 5,	//!< a channel that users can follow and crosspost into their own server
	/**
	 * @brief a channel in which game developers can sell their game on Discord
	 * @deprecated store channels are deprecated by Discord
	 */
	CHANNEL_STORE		= 6,
	CHANNEL_NEWS_THREAD	= 10,	//!< a temporary sub-channel within a GUILD_NEWS channel
	CHANNEL_PUBLIC_THREAD	= 11,	//!< a temporary sub-channel within a GUILD_TEXT channel
	CHANNEL_PRIVATE_THREAD	= 12,	//!< a temporary sub-channel within a GUILD_TEXT channel that is only viewable by those invited and those with the MANAGE_THREADS permission
	CHANNEL_STAGE		= 13,	//!< a "stage" channel, like a voice channel with one authorised speaker
	CHANNEL_DIRECTORY	= 14,   //!< the channel in a [hub](https://support.discord.com/hc/en-us/articles/4406046651927-Discord-Student-Hubs-FAQ) containing the listed servers
	CHANNEL_FORUM		= 15	//!< forum channel, coming soon(tm)
};

/** @brief Our flags as stored in the object
 * @note The bottom four bits of this flag are reserved to contain the channel_type values
 * listed above as provided by Discord. If discord add another value > 15, we will have to
 * shuffle these values upwards by one bit.
 */
enum channel_flags : uint8_t {
	/// NSFW Gated Channel
	c_nsfw =		0b00010000,
	/// Video quality forced to 720p
	c_video_quality_720p =	0b00100000,
	/// Lock permissions (only used when updating channel positions)
	c_lock_permissions =	0b01000000,
	/// Thread pinned in a forum (type 15) channel
	c_pinned_thread =	0b10000000,
};

/**
 * @brief The flags in discord channel's raw "flags" field. We use these for serialisation only, right now. Might be better to create a new field than to make the existing channel::flags from uint8_t to uint16_t, if discord adds more flags in future.
 */
enum discord_channel_flags : uint8_t {
	/// Thread pinned in a forum (type 15) channel
	dc_pinned_thread = 0b00000001,
};

/**
 * @brief channel permission overwrite types
 */
enum overwrite_type : uint8_t {
	/// Role
	ot_role = 0,
	/// Member
	ot_member = 1
};

/**
 * @brief Channel permission overwrites
 */
struct DPP_EXPORT permission_overwrite {
	/// ID of the role or the member
	snowflake id;
	/// Bitmask of allowed permissions
	permission allow;
	/// Bitmask of denied permissions
	permission deny;
	/// Type of overwrite. See dpp::overwrite_type
	uint8_t type;

	/**
	 * @brief Construct a new permission_overwrite object
	 */
	permission_overwrite();

	/**
	 * @brief Construct a new permission_overwrite object
	 * @param id ID of the role or the member to create the overwrite for
	 * @param allow Bitmask of allowed permissions (refer to enum dpp::permissions) for this user/role in this channel
	 * @param deny Bitmask of denied permissions (refer to enum dpp::permissions) for this user/role in this channel
	 * @param type Type of overwrite
	 */
	permission_overwrite(snowflake id, uint64_t allow, uint64_t deny, overwrite_type type);
};


/**
 * @brief metadata for threads
 */
struct DPP_EXPORT thread_metadata {
	/// When the thread was archived
	time_t archive_timestamp;
	/// The duration after a thread will archive
	uint16_t auto_archive_duration;
	/// Whether a thread is archived
	bool archived;
	/// Whether a thread is locked
	bool locked;
	/// Whether non-moderators can add other non-moderators 
	bool invitable;
};

/**
 * @brief represents membership of a user with a thread
 */
struct DPP_EXPORT thread_member  
{
	/// ID of the thread member is part of
	snowflake thread_id;
	/// ID of the member 
	snowflake user_id;
	/// When the user joined the thread
	time_t joined;
	/// Flags bitmap
	uint32_t flags;

	/**
	 * @brief Read struct values from a json object 
	 * @param j json to read values from
	 * @return A reference to self	
	 */
	 thread_member& fill_from_json(nlohmann::json* j);
};

/** @brief A group of thread member objects*/
typedef std::unordered_map<snowflake, thread_member> thread_member_map;

/**
 * @brief A definition of a discord channel
 * There are one of these for every channel type except threads. Threads are
 * special snowflakes. Get it? A Discord pun. Hahaha. .... I'll get my coat.
 */ 
class DPP_EXPORT channel : public managed, public json_interface<channel>  {
public:
	/** Channel name */
	std::string name;

	/** Channel topic */
	std::string topic;

	/**
	 * @brief Voice region if set for voice channel, otherwise empty string
	 */
	std::string rtc_region;

	/** DM recipients */
	std::vector<snowflake> recipients;

	/** Permission overwrites to apply to base permissions */
	std::vector<permission_overwrite> permission_overwrites;

	/**
	 * @brief Channel icon (for group DMs)
	 */
	utility::iconhash icon;

	/**
	 * @brief Channel banner (boost level locked)
	 */
	utility::iconhash banner;

	/** User ID of owner for group DMs */
	snowflake owner_id;

	/** Parent ID (category) */
	snowflake parent_id;

	/** Guild id of the guild that owns the channel */
	snowflake guild_id;

	/** ID of last message to be sent to the channel */
	snowflake last_message_id;

	/** Timestamp of last pinned message */
	time_t last_pin_timestamp;

	/**
	 * @brief This is only filled when the channel is part of the `resolved` set
	 * sent within an interaction. Any other time it contains zero. When filled,
	 * it contains the calculated permission bitmask of the user issuing the command
	 * within this channel.
	 */
	permission permissions;

	/** Sorting position, lower number means higher up the list */
	uint16_t position;

	/** the bitrate (in bits) of the voice channel */
	uint16_t bitrate;

	/** amount of seconds a user has to wait before sending another message (0-21600); bots, as well as users with the permission manage_messages or manage_channel, are unaffected*/
	uint16_t rate_limit_per_user;

	/** Flags bitmap */
	uint8_t flags;
	
	/** Maximum user limit for voice channels (0-99) */
	uint8_t user_limit;

	/** Constructor */
	channel();

	/** Destructor */
	virtual ~channel();

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	 channel& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build json for this channel object
	 * 
	 * @param with_id include the ID in the json
	 * @return std::string JSON string
	 */
	virtual std::string build_json(bool with_id = false) const;

	/**
	 * @brief Set name of this channel object
	 *
	 * @param name Name to set
	 * @return Reference to self, so these method calls may be chained 
	 *
	 * @note name will be truncated to 100 chars, if longer
	 * @throw dpp::length_exception if length < 1
	 */
	channel& set_name(const std::string& name);

	/**
	 * @brief Set topic of this channel object
	 *
	 * @param topic Topic to set
	 * @return Reference to self, so these method calls may be chained 
	 *
	 * @note topic will be truncated to 1024 chars, if longer
	 */
	channel& set_topic(const std::string& topic);

	/**
	 * @brief Set flags for this channel object
	 *
	 * @param flags Flag bitmask to set
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& set_flags(const uint16_t flags);

	/**
	 * @brief Add (bitwise OR) a flag to this channel object
	 * 	
	 * @param flag Flag bit to set
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& add_flag(const channel_flags flag);

	/**
	 * @brief Remove (bitwise NOT AND) a flag from this channel object
	 * 	
	 * @param flag Flag bit to set
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& remove_flag(const channel_flags flag);

	/**
	 * @brief Set position of this channel object
	 *
	 * @param position Position to set
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& set_position(const uint16_t position);

	/**
	 * @brief Set guild_id of this channel object
	 *
	 * @param guild_id Guild ID to set
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& set_guild_id(const snowflake guild_id);

	/**
	 * @brief Set parent_id of this channel object
	 *
	 * @param parent_id Parent ID to set
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& set_parent_id(const snowflake parent_id);

	/**
	 * @brief Set user_limit of this channel object
	 *
	 * @param user_limit Limit to set
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& set_user_limit(const uint8_t user_limit);

	/**
	 * @brief Set bitrate of this channel object
	 *
	 * @param bitrate Bitrate to set
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& set_bitrate(const uint16_t bitrate);

	/**
	 * @brief Set nsfw property of this channel object
	 *
	 * @param is_nsfw true, if channel is nsfw
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& set_nsfw(const bool is_nsfw);

	/**
	 * @brief Set lock permissions property of this channel object
	 * Used only with the reorder channels method
	 *
	 * @param is_lock_permissions true, if we are to inherit permissions from the category
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& set_lock_permissions(const bool is_lock_permissions);

	/**
	 * @brief Set rate_limit_per_user of this channel object
	 *
	 * @param rate_limit_per_user rate_limit_per_user (slowmode in sec) to set
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& set_rate_limit_per_user(const uint16_t rate_limit_per_user);

	/**
	 * @brief Add a permission_overwrite to this channel object
	 * 
	 * @param id ID of the role or the member you want to add overwrite for
	 * @param type type of overwrite
	 * @param allowed_permissions bitmask of allowed permissions (refer to enum dpp::permissions) for this user/role in this channel
	 * @param denied_permissions bitmask of denied permissions (refer to enum dpp::permissions) for this user/role in this channel
	 *
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& add_permission_overwrite(const snowflake id, const overwrite_type type, const uint64_t allowed_permissions, const uint64_t denied_permissions);

	/**
	 * @brief Get the mention ping for the channel
	 * 
	 * @return std::string mention
	 */
	std::string get_mention() const;

	/**
	 * @brief Get the user permissions for a user on this channel
	 * 
	 * @param member The user to return permissions for
	 * @return permission Permissions bitmask made of bits in dpp::permissions.
	 * Note that if the user is not on the channel or the guild is
	 * not in the cache, the function will always return 0.
	 */
	permission get_user_permissions(const class user* member) const;

	/**
	 * @brief Return a map of members on the channel, built from the guild's
	 * member list based on which members have the VIEW_CHANNEL permission.
	 * Does not return reliable information for voice channels, use
	 * dpp::channel::get_voice_members() instead for this.
	 * @return A map of guild members keyed by user id.
	 */
	std::map<snowflake, class guild_member*> get_members();

	/**
	 * @brief Get a map of members in this channel, if it is a voice channel.
	 * The map is keyed by snowflake id of the user.
	 * 
	 * @return std::map<snowflake, voicestate> The voice members of the channel
	 */
	std::map<snowflake, voicestate> get_voice_members();

	/**
	 * @brief Get the channel's banner url if they have one, otherwise returns an empty string
	 *
	 * @param size The size of the banner in pixels. It can be any power of two between 16 and 4096. if not specified, the default sized banner is returned.
	 * @return std::string banner url or empty string
	 */
	std::string get_banner_url(uint16_t size = 0) const;

	/**
	 * @brief Get the channel's icon url (if its a group DM), otherwise returns an empty string
	 *
	 * @param size The size of the icon in pixels. It can be any power of two between 16 and 4096. if not specified, the default sized icon is returned.
	 * @return std::string icon url or empty string
	 */
	std::string get_icon_url(uint16_t size = 0) const;

	/**
	 * @brief Returns true if the channel is NSFW gated
	 * 
	 * @return true if NSFW
	 */
	bool is_nsfw() const;

	/**
	 * @brief Returns true if the permissions are to be synched with the category it is in.
	 * Used only and set manually when using the reorder channels method.
	 * 
	 * @return true if keeping permissions
	 */
	bool is_locked_permissions() const;

	/**
	 * @brief Returns true if the channel is a text channel
	 * 
	 * @return true if text channel
	 */
	bool is_text_channel() const;

	/**
	 * @brief Returns true if the channel is a DM
	 * 
	 * @return true if is a DM
	 */
	bool is_dm() const;

	/**
	 * @brief Returns true if the channel is a voice channel
	 * 
	 * @return true if voice channel
	 */
	bool is_voice_channel() const;

	/**
	 * @brief Returns true if the channel is a group DM channel
	 * 
	 * @return true if group DM
	 */
	bool is_group_dm() const;

	/**
	 * @brief Returns true if the channel is a category
	 * 
	 * @return true if a category
	 */
	bool is_category() const;

	/**
	 * @brief Returns true if the channel is a forum
	 * @note This feature is not implemented by Discord yet and the name is subject to possible change!
	 * 
	 * @return true if a category
	 */
	bool is_forum() const;

	/**
	 * @brief Returns true if the channel is a news channel
	 * 
	 * @return true if news channel
	 */
	bool is_news_channel() const;

	/**
	 * @brief Returns true if the channel is a store channel
	 * @deprecated store channels are deprecated by Discord
	 * 
	 * @return true if store channel
	 */
	bool is_store_channel() const;

	/**
	 * @brief Returns true if the channel is a stage channel
	 * 
	 * @return true if stage channel
	 */
	bool is_stage_channel() const;

	/**
	 * @brief Returns true if video quality is auto
	 * 
	 * @return true if video quality is auto
	 */
	bool is_video_auto() const;

	/**
	 * @brief Returns true if video quality is 720p
	 * 
	 * @return true if video quality is 720p
	 */
	bool is_video_720p() const;

	/**
	 * @brief Returns true if channel is a pinned thread in forum
	 *
	 * @return true, if channel is a pinned thread in forum
	 */
	bool is_pinned_thread() const;

};

/** @brief A definition of a discord thread.
 * A thread is a superset of a channel. Not to be confused with `std::thread`!
 */
class DPP_EXPORT thread : public channel {
public:
	/**
	 * @brief Thread member of current user if joined to the thread.
	 * Note this is only set by certain api calls otherwise contains default data
	 */
	thread_member member;

	/** Thread metadata (threads) */
	thread_metadata metadata;

	/** Approximate count of messages in a thread (threads) */
	uint8_t message_count;

	/** Approximate count of members in a thread (threads) */
	uint8_t member_count;

	/**
	 * @brief Construct a new thread object
	 */
	thread();

	/**
	 * @brief Returns true if the channel is a news thread
	 *
	 * @return true if news thread
	 */
	bool is_news_thread() const;

	/**
	 * @brief Returns true if the channel is a public thread
	 *
	 * @return true if public thread
	 */
	bool is_public_thread() const;

	/**
	 * @brief Returns true if the channel is a private thread
	 *
	 * @return true if private thread
	 */
	bool is_private_thread() const;

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	thread& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Destroy the thread object
	 */
	virtual ~thread();

	/**
	 * @brief Build json for this thread object
	 * 
	 * @param with_id include the ID in the json
	 * @return std::string JSON string
	 */
	std::string build_json(bool with_id = false) const;

};


/**
 * @brief Serialize a thread_metadata object to json
 *
 * @param j JSON object to serialize to
 * @param tmdata object to serialize
 */
void to_json(nlohmann::json& j, const thread_metadata& tmdata);

/**
 * @brief Serialize a permission_overwrite object to json
 *
 * @param j JSON object to serialize to
 * @param po object to serialize
 */
void to_json(nlohmann::json& j, const permission_overwrite& po);

/**
 * @brief A group of channels
 */
typedef std::unordered_map<snowflake, channel> channel_map;

/**
 * @brief A group of threads
 */
typedef std::unordered_map<snowflake, thread> thread_map;

};

