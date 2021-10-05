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
#include <dpp/json_fwd.hpp>

namespace dpp {

/** @brief Flag integers as received from and sent to discord */
enum channel_type {
GUILD_TEXT	= 0,	//!< a text channel within a server
DM		= 1,	//!< a direct message between users
GUILD_VOICE	= 2,	//!< a voice channel within a server
GROUP_DM	= 3,	//!< a direct message between multiple users
GUILD_CATEGORY	= 4,	//!< an organizational category that contains up to 50 channels
GUILD_NEWS	= 5,	//!< a channel that users can follow and crosspost into their own server
GUILD_STORE	= 6,	//!< a channel in which game developers can sell their game on Discord
GUILD_NEWS_THREAD	= 10, //!< a temporary sub-channel within a GUILD_NEWS channel
GUILD_PUBLIC_THREAD	= 11, //!< a temporary sub-channel within a GUILD_TEXT channel
GUILD_PRIVATE_THREAD	= 12, //!< a temporary sub-channel within a GUILD_TEXT channel that is only viewable by those invited and those with the MANAGE_THREADS permission
GUILD_STAGE	= 13	//!< a "stage" channel, like a voice channel with one authorised speaker
};
/** @brief Our flags as stored in the object */
enum channel_flags {
	/// NSFW Gated Channel
	c_nsfw =			0b00000001,
	/// Text channel
	c_text =			0b00000010,
	/// Direct Message
	c_dm =				0b00000100,
	/// Voice channel
	c_voice =			0b00001000,
	/// Group
	c_group =			0b00010000,
	/// Category
	c_category =		0b00100000,
	/// News channel
	c_news =			0b01000000,
	/// Store page
	c_store =			0b10000000,
	/// Stage channel
	c_stage =			0b11000000,
	/// News thread
	c_news_thread =		0b11100000,
	/// Public thread
	c_public_thread = 	0b11110000,
	/// Private thread
	c_private_thread =	0b11111000
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
 * @brief channel permission overwrites
 */
struct DPP_EXPORT permission_overwrite {
	/// Overwrite id
	snowflake id;
	/// Overwrite type
	uint8_t type;
	/// Allow mask
	uint64_t allow;
	/// Deny mask
	uint64_t deny;
};


/**
 * @brief metadata for threads
 */
struct DPP_EXPORT thread_metadata {
	/// Whether a thread is archived
	bool archived;
	/// When the thread was archived
	time_t archive_timestamp;
	/// The duration after a thread will archive
	uint16_t auto_archive_duration;
	/// Whether a thread is locked
	bool locked;
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

/** @brief A definition of a discord channel */
class DPP_EXPORT channel : public managed {
public:
	/** Flags bitmap */
	uint8_t flags;
	
	/** Guild id of the guild that owns the channel */
	snowflake guild_id;

	/** Sorting position, lower number means higher up the list */
	uint16_t position;

	/** Channel name */
	std::string name;

	/** Channel topic */
	std::string topic;

	/** ID of last message to be sent to the channel */
	snowflake last_message_id;

	/** Maximum user limit for voice channels (0-99) */
	uint8_t user_limit;

	/** Rate limit in kilobits per second for voice channels */
	uint16_t rate_limit_per_user;

	/** User ID of owner for group DMs */
	snowflake owner_id;

	/** Parent ID (category) */
	snowflake parent_id;

	/** Timestamp of last pinned message */
	time_t last_pin_timestamp;

	/** DM recipients */
	std::vector<snowflake> recipients;

	/** Permission overwrites to apply to base permissions */
	std::vector<permission_overwrite> permission_overwrites;

	/** Approximate count of messages in a thread (threads) */
	uint8_t message_count;

	/** Approximate count of members in a thread (threads) */
	uint8_t member_count;

	/** Thread metadata (threads) */
	thread_metadata metadata;

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
	std::string build_json(bool with_id = false) const;

	/**
	 * @brief Get the user permissions for a user on this channel
	 * 
	 * @param member The user to return permissions for
	 * @return uint64_t Permissions bitmask made of bits in role_permissions.
	 * Note that if the user is not on the channel or the guild is
	 * not in the cache, the function will always return 0.
	 */
	uint64_t get_user_permissions(const class user* member) const;

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
	 * @brief Returns true if the channel is NSFW gated
	 * 
	 * @return true if NSFW
	 */
	bool is_nsfw() const;

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
	 * @brief Returns true if the channel is a news channel
	 * 
	 * @return true if news channel
	 */
	bool is_news_channel() const;

	/**
	 * @brief Returns true if the channel is a store channel
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
};

/**
 * @brief A group of channels
 */
typedef std::unordered_map<snowflake, channel> channel_map;

};

