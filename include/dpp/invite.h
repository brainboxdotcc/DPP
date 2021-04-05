#pragma once

#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

class invite {
public:
	std::string code;
	snowflake guild_id;
	snowflake channel_id;
	snowflake inviter_id;
	snowflake target_user_id;
	uint8_t target_user_type;
	uint32_t approximate_presence_count;
	uint32_t approximate_member_count;
	uint32_t max_age;
	uint32_t max_uses;
	bool temporary;
	bool unique;

	invite();
	~invite();
	invite& fill_from_json(nlohmann::json* j);
	std::string build_json() const;

};

/** A container of invites */
typedef std::unordered_map<snowflake, invite> invite_map;


};
