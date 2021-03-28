#pragma once

namespace dpp {

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

class channel {
public:
	snowflake id;
	uint8_t flags;
	snowflake guild_id;
	uint16_t position;
	std::string name;
	std::string topic;
	snowflake last_message_id;
	uint32_t user_limit;
	uint16_t rate_limit_per_user;
	snowflake owner_id;
	snowflake parent_id;
	time_t last_pin_timestamp;

	channel();
	~channel();
};

typedef std::unordered_map<snowflake, channel*> channel_map;

};

