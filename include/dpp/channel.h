#pragma once

#include <dpp/json_fwd.hpp>

namespace dpp {

/** @brief Flag integers as received from and sent to discord */
#define GUILD_TEXT	0	// a text channel within a server
#define DM		1	// a direct message between users
#define GUILD_VOICE	2	// a voice channel within a server
#define GROUP_DM	3	// a direct message between multiple users
#define GUILD_CATEGORY	4	// an organizational category that contains up to 50 channels
#define GUILD_NEWS	5	// a channel that users can follow and crosspost into their own server
#define GUILD_STORE	6	// a channel in which game developers can sell their game on Discord

/** @brief Our flags as stored in the object */
enum channel_flags {
	c_nsfw =		0b00000001,
	c_text =		0b00000010,
	c_dm =			0b00000100,
	c_voice =		0b00001000,
	c_group =		0b00010000,
	c_category =		0b00100000,
	c_news =		0b01000000,
	c_store =		0b10000000
};

/** @brief A definition of a discord channel */
class channel : public managed {
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

	/** Maximum user limit for voice channels */
	uint32_t user_limit;

	/** Rate limit in kilobits per second for voice channels */
	uint16_t rate_limit_per_user;

	/** User ID of owner for group DMs */
	snowflake owner_id;

	/** Parent ID (category) */
	snowflake parent_id;

	/** Timestamp of last pinned message */
	time_t last_pin_timestamp;

	/** Constructor */
	channel();

	/** Destructor */
	~channel();

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	channel& fill_from_json(nlohmann::json* j);
	std::string build_json(bool with_id = false) const;

	bool is_nsfw() const;
	bool is_text_channel() const;
	bool is_dm() const;
	bool is_voice_channel() const;
	bool is_group_dm() const;
	bool is_category() const;
	bool is_news_channel() const;
	bool is_store_channel() const;
};

/**
 * @brief A group of channels
 */
typedef std::unordered_map<snowflake, channel> channel_map;

};

