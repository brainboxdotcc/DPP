#pragma once

namespace dpp {

enum role_flags {
	r_hoist =		0b00000001,
	r_managed =		0b00000010,
	r_mentionable =		0b00000100,
	r_premium_subscriber =	0b00001000,
};

class role {
public:
	snowflake id;
	std::string name;
	uint32_t colour;
	uint8_t position;
	uint32_t permissions;
	uint8_t flags;
	snowflake integration_id;
	snowflake bot_id;

	role();
	~role();
};

typedef std::unordered_map<snowflake, role*> role_map;

};
