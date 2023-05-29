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
#include <dpp/message.h>
#include <dpp/json_fwd.h>
#include <dpp/permissions.h>
#include <dpp/json_interface.h>
#include <unordered_map>
#include <variant>

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
	CHANNEL_ANNOUNCEMENT	= 5,	//!< a channel that users can follow and crosspost into their own server
	/**
	 * @brief a channel in which game developers can sell their game on Discord
	 * @deprecated store channels are deprecated by Discord
	 */
	CHANNEL_STORE		= 6,
	CHANNEL_ANNOUNCEMENT_THREAD	= 10,	//!< a temporary sub-channel within a GUILD_ANNOUNCEMENT channel
	CHANNEL_PUBLIC_THREAD	= 11,	//!< a temporary sub-channel within a GUILD_TEXT or GUILD_FORUM channel
	CHANNEL_PRIVATE_THREAD	= 12,	//!< a temporary sub-channel within a GUILD_TEXT channel that is only viewable by those invited and those with the MANAGE_THREADS permission
	CHANNEL_STAGE		= 13,	//!< a "stage" channel, like a voice channel with one authorised speaker
	CHANNEL_DIRECTORY	= 14,   //!< the channel in a [hub](https://support.discord.com/hc/en-us/articles/4406046651927-Discord-Student-Hubs-FAQ) containing the listed servers
	CHANNEL_FORUM		= 15	//!< forum channel that can only contain threads
};

/** @brief Our flags as stored in the object
 * @note The bottom four bits of this flag are reserved to contain the channel_type values
 * listed above as provided by Discord. If discord add another value > 15, we will have to
 * shuffle these values upwards by one bit.
 */
enum channel_flags : uint16_t {
	/// NSFW Gated Channel
	c_nsfw =		0b0000000000010000,
	/// Video quality forced to 720p
	c_video_quality_720p =	0b0000000000100000,
	/// Lock permissions (only used when updating channel positions)
	c_lock_permissions =	0b0000000001000000,
	/// Thread is pinned to the top of its parent forum channel
	c_pinned_thread =	0b0000000010000000,
	/// Whether a tag is required to be specified when creating a thread in a forum channel. Tags are specified in the thread::applied_tags field.
	c_require_tag =		0b0000000100000000,
	/* Note that the 9th and 10th bit are used for the forum layout type */
};

/**
 * @brief The flags in discord channel's raw "flags" field. We use these for serialisation only, right now. Might be better to create a new field than to make the existing channel::flags from uint8_t to uint16_t, if discord adds more flags in future.
 */
enum discord_channel_flags : uint8_t {
	/// Thread is pinned to the top of its parent forum channel
	dc_pinned_thread = 1 << 1,
	/// Whether a tag is required to be specified when creating a thread in a forum channel. Tags are specified in the thread::applied_tags field.
	dc_require_tag =   1 << 4,
};

/**
 * @brief Types for sort posts in a forum channel
 */
enum default_forum_sort_order_t : uint8_t {
	/// Sort forum posts by activity (default)
	so_latest_activity = 0,
	/// Sort forum posts by creation time (from most recent to oldest)
	so_creation_date = 1,
};

/**
 * @brief Types of forum layout views that indicates how the threads in a forum channel will be displayed for users by default
 */
enum forum_layout_type : uint8_t {
	fl_not_set = 0, //!< No default has been set for the forum channel
	fl_list_view = 1, //!< Display posts as a list
	fl_gallery_view = 2, //!< Display posts as a collection of tiles
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
	/// Timestamp when the thread's archive status was last changed, used for calculating recent activity
	time_t archive_timestamp;
	/// The duration in minutes to automatically archive the thread after recent activity, can be set to: 60, 1440, 4320, 10080
	uint16_t auto_archive_duration;
	/// Whether a thread is archived
	bool archived;
	/// Whether a thread is locked. When a thread is locked, only users with `MANAGE_THREADS` can unarchive it
	bool locked;
	/// Whether non-moderators can add other non-moderators. Only for private threads
	bool invitable;
};

/**
 * @brief Auto archive duration of threads which will stop showing in the channel list after the specified period of inactivity.
 * Defined as an enum to fit into 1 byte. Internally it'll be translated to minutes to match the API
 */
enum auto_archive_duration_t : uint8_t {
	/// Auto archive duration of 1 hour. (60 minutes)
	arc_1_hour = 1,
	/// Auto archive duration of 1 day. (1440 minutes)
	arc_1_day = 2,
	/// Auto archive duration of 3 days. (4320 minutes)
	arc_3_days = 3,
	/// Auto archive duration of 1 week. (10080 minutes)
	arc_1_week = 4,
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
	/// The time when user last joined the thread
	time_t joined;
	/// Any user-thread settings, currently only used for notifications
	uint32_t flags;

	/**
	 * @brief Read struct values from a json object 
	 * @param j json to read values from
	 * @return A reference to self	
	 */
	 thread_member& fill_from_json(nlohmann::json* j);
};

/**
 * @brief Represents a tag that is able to be applied to a thread in a forum channel
 */
struct DPP_EXPORT forum_tag : public managed {
	/** The name of the tag (0-20 characters) */
	std::string name;
	/** The emoji of the tag. Contains either nothing, the id of a guild's custom emoji or the unicode character of the emoji */
	std::variant<std::monostate, snowflake, std::string> emoji;
	/** Whether this tag can only be added to or removed from threads by a member with the `MANAGE_THREADS` permission */
	bool moderated;

	/** Constructor */
	forum_tag();

	/**
	 * @brief Constructor
	 *
	 * @param name The name of the tag. It will be truncated to the maximum length of 20 UTF-8 characters.
	 */
	forum_tag(const std::string& name);

	/** Destructor */
	virtual ~forum_tag();

	/**
	 * @brief Read struct values from a json object
	 * @param j json to read values from
	 * @return A reference to self
	 */
	forum_tag& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build json for this forum_tag object
	 *
	 * @param with_id include the ID in the json
	 * @return std::string JSON string
	 */
	std::string build_json(bool with_id = false) const;

	/**
	 * @brief Set name of this forum_tag object
	 *
	 * @param name Name to set
	 * @return Reference to self, so these method calls may be chained
	 *
	 * @note name will be truncated to 20 chars, if longer
	 */
	forum_tag& set_name(const std::string& name);
};

/**
 * @brief A group of thread member objects. the key is the user_id of the dpp::thread_member
 */
typedef std::unordered_map<snowflake, thread_member> thread_member_map;

/**
 * @brief A definition of a discord channel.
 * There are one of these for every channel type except threads. Threads are
 * special snowflakes. Get it? A Discord pun. Hahaha. .... I'll get my coat.
 */ 
class DPP_EXPORT channel : public managed, public json_interface<channel>  {
public:
	/** Channel name (1-100 characters) */
	std::string name;

	/** Channel topic (0-4096 characters for forum channels, 0-1024 characters for all others) */
	std::string topic;

	/**
	 * @brief Voice region if set for voice channel, otherwise empty string
	 */
	std::string rtc_region;

	/** DM recipients */
	std::vector<snowflake> recipients;

	/** Permission overwrites to apply to base permissions */
	std::vector<permission_overwrite> permission_overwrites;

	/** A set of tags that can be used in a forum channel */
	std::vector<forum_tag> available_tags;

	/**
	 * @brief The emoji to show as the default reaction button on a forum post.
	 * Contains either nothing, the id of a guild's custom emoji or the unicode character of the emoji
	 */
	std::variant<std::monostate, snowflake, std::string> default_reaction;

	/**
	 * @brief Channel icon (for group DMs)
	 */
	utility::iconhash icon;

	/** User ID of the creator for group DMs or threads */
	snowflake owner_id;

	/** Parent ID (for guild channels: id of the parent category, for threads: id of the text channel this thread was created) */
	snowflake parent_id;

	/** Guild id of the guild that owns the channel */
	snowflake guild_id;

	/** ID of last message to be sent to the channel (may not point to an existing or valid message or thread) */
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

	/** the bitrate (in kilobits) of the voice channel */
	uint16_t bitrate;

	/** amount of seconds a user has to wait before sending another message (0-21600); bots, as well as users with the permission manage_messages or manage_channel, are unaffected*/
	uint16_t rate_limit_per_user;

	/** The initial `rate_limit_per_user` to set on newly created threads in a channel. This field is copied to the thread at creation time and does not live update */
	uint16_t default_thread_rate_limit_per_user;

	/**
	 * @brief Default duration, copied onto newly created threads. Used by the clients, not the API.
	 * Threads will stop showing in the channel list after the specified period of inactivity. Defaults to dpp::arc_1_day
	 */
	auto_archive_duration_t default_auto_archive_duration;

	/** the default sort order type used to order posts in forum channels */
	default_forum_sort_order_t default_sort_order;

	/** Flags bitmap (dpp::channel_flags) */
	uint16_t flags;
	
	/** Maximum user limit for voice channels (0-99) */
	uint8_t user_limit;

	/** Constructor */
	channel();

	/** Destructor */
	virtual ~channel();

	/**
	* @brief Create a mentionable channel.
	* @param id The ID of the channel.
	* @return std::string The formatted mention of the channel.
	*/
	static std::string get_mention(const snowflake& id);

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
	 * @brief Set type of this channel object
	 *
	 * @param type Channel type to set
	 * @return Reference to self, so these method calls may be chained
	 */
	channel& set_type(channel_type type);

	/**
	 * @brief Set the default forum layout type for the forum channel
	 *
	 * @param layout_type The layout type
	 * @return Reference to self, so these method calls may be chained
	 */
	channel& set_default_forum_layout(forum_layout_type layout_type);

	/**
	 * @brief Set the default forum sort order for the forum channel
	 *
	 * @param sort_order The sort order
	 * @return Reference to self, so these method calls may be chained
	 */
	channel& set_default_sort_order(default_forum_sort_order_t sort_order);

	/**
	 * @brief Set flags for this channel object
	 *
	 * @param flags Flag bitmask to set from dpp::channel_flags
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& set_flags(const uint16_t flags);

	/**
	 * @brief Add (bitwise OR) a flag to this channel object
	 * 	
	 * @param flag Flag bit to add from dpp::channel_flags
	 * @return Reference to self, so these method calls may be chained 
	 */
	channel& add_flag(const channel_flags flag);

	/**
	 * @brief Remove (bitwise NOT AND) a flag from this channel object
	 * 	
	 * @param flag Flag bit to remove from dpp::channel_flags
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
	 * @param bitrate Bitrate to set (in kilobits)
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
	 * @brief Add permission overwrites for a user or role.
	 * If the channel already has permission overwrites for the passed target, the existing ones will be adjusted by the passed permissions
	 *
	 * @param target ID of the role or the member you want to adjust overwrites for
	 * @param type type of overwrite
	 * @param allowed_permissions bitmask of dpp::permissions you want to allow for this user/role in this channel. Note: You can use the dpp::permission class
	 * @param denied_permissions bitmask of dpp::permissions you want to deny for this user/role in this channel. Note: You can use the dpp::permission class
	 *
	 * @return Reference to self, so these method calls may be chained
	 */
	channel& add_permission_overwrite(const snowflake target, const overwrite_type type, const uint64_t allowed_permissions, const uint64_t denied_permissions);
	/**
	 * @brief Set permission overwrites for a user or role on this channel object. Old permission overwrites for the target will be overwritten
	 *
	 * @param target ID of the role or the member you want to set overwrites for
	 * @param type type of overwrite
	 * @param allowed_permissions bitmask of allowed dpp::permissions for this user/role in this channel. Note: You can use the dpp::permission class
	 * @param denied_permissions bitmask of denied dpp::permissions for this user/role in this channel. Note: You can use the dpp::permission class
	 *
	 * @return Reference to self, so these method calls may be chained
	 *
	 * @note If both `allowed_permissions` and `denied_permissions` parameters are 0, the permission overwrite for the target will be removed
	 */
	channel& set_permission_overwrite(const snowflake target, const overwrite_type type, const uint64_t allowed_permissions, const uint64_t denied_permissions);
	/**
	 * @brief Remove channel specific permission overwrites of a user or role
	 *
	 * @param target ID of the role or the member you want to remove permission overwrites of
	 * @param type type of overwrite
	 *
	 * @return Reference to self, so these method calls may be chained
	 */
	channel& remove_permission_overwrite(const snowflake target, const overwrite_type type);

	/**
	 * @brief Get the channel type
	 *
	 * @return channel_type Channel type
	 */
	channel_type get_type() const;

	/**
	 * @brief Get the default forum layout type used to display posts in forum channels
	 *
	 * @return forum_layout_types Forum layout type
	 */
	forum_layout_type get_default_forum_layout() const;

	/**
	 * @brief Get the mention ping for the channel
	 * 
	 * @return std::string mention
	 */
	std::string get_mention() const;

	/**
	 * @brief Get the overall permissions for a member in this channel, including channel overwrites, role permissions and admin privileges.
	 * 
	 * @param user The user to resolve the permissions for
	 * @return permission Permission overwrites for the member. Made of bits in dpp::permissions.
	 * @note Requires role cache to be enabled (it's enabled by default).
	 *
	 * @note This is an alias for guild::permission_overwrites and searches for the guild in the cache,
	 * so consider using guild::permission_overwrites if you already have the guild object.
	 *
	 * @warning The method will search for the guild member in the cache by the users id.
	 * If the guild member is not in cache, the method will always return 0.
	 */
	permission get_user_permissions(const class user* user) const;

	/**
	 * @brief Get the overall permissions for a member in this channel, including channel overwrites, role permissions and admin privileges.
	 *
	 * @param member The member to resolve the permissions for
	 * @return permission Permission overwrites for the member. Made of bits in dpp::permissions.
	 * @note Requires role cache to be enabled (it's enabled by default).
	 *
	 * @note This is an alias for guild::permission_overwrites and searches for the guild in the cache,
	 * so consider using guild::permission_overwrites if you already have the guild object.
	 */
	permission get_user_permissions(const class guild_member &member) const;

	/**
	 * @brief Return a map of members on the channel, built from the guild's
	 * member list based on which members have the VIEW_CHANNEL permission.
	 * Does not return reliable information for voice channels, use
	 * dpp::channel::get_voice_members() instead for this.
	 * @return A map of guild members keyed by user id.
	 * @note If the guild this channel belongs to is not in the cache, the function will always return 0.
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
	 * @brief Get the channel's icon url (if its a group DM), otherwise returns an empty string
	 *
	 * @param size The size of the icon in pixels. It can be any power of two between 16 and 4096,
	 * otherwise the default sized icon is returned.
	 * @param format The format to use for the avatar. It can be one of `i_webp`, `i_jpg` or `i_png`.
	 * @return std::string icon url or an empty string, if required attributes are missing or an invalid format was passed
	 */
	std::string get_icon_url(uint16_t size = 0, const image_type format = i_png) const;

	/**
	 * @brief Returns true if the channel is NSFW gated
	 * 
	 * @return true if NSFW
	 */
	bool is_nsfw() const;

	/**
	 * @brief Returns true if the permissions are to be synced with the category it is in.
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
	 * 
	 * @return true if a forum
	 */
	bool is_forum() const;

	/**
	 * @brief Returns true if the channel is an announcement channel
	 * 
	 * @return true if announcement channel
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

	/**
	 * @brief Returns true if a tag is required to be specified when creating a thread in a forum channel
	 *
	 * @return true, if a tag is required to be specified when creating a thread in a forum channel
	 */
	bool is_tag_required() const;

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

	/** Created message. Only filled within the cluster::thread_create_in_forum() method */
	message msg;

	/**
	 * A list of dpp::forum_tag IDs that have been applied to a thread in a forum channel
	 */
	std::vector<snowflake> applied_tags;

	/**
	 * @brief Number of messages ever sent in the thread.
	 * It's similar to thread::message_count on message creation, but will not decrement the number when a message is deleted
	 */
	uint32_t total_messages_sent;

	/**
	 * @brief Number of messages (not including the initial message or deleted messages) of the thread.
	 * For threads created before July 1, 2022, the message count is inaccurate when it's greater than 50.
	 */
	uint8_t message_count;

	/** Approximate count of members in a thread (stops counting at 50) */
	uint8_t member_count;

	/**
	 * @brief Construct a new thread object
	 */
	thread();

	/**
	 * @brief Returns true if the thread is within an announcement channel
	 *
	 * @return true if announcement thread
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

/**
 * @brief A thread alongside the bot's optional thread_member object tied to it
 */
struct active_thread_info {
	/**
	 * @brief The thread object
	 */
	thread active_thread;

	/**
	 * @brief The bot as a thread member, only present if the bot is in the thread
	 */
	std::optional<thread_member> bot_member;
};

/**
 * @brief A map of threads alongside optionally the thread_member tied to the bot if it is in the thread. The map's key is the thread id. Returned from the cluster::threads_get_active method
 */
using active_threads = std::map<snowflake, active_thread_info>;

};

