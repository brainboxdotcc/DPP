#pragma once

#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

class dtemplate {
public:
	std::string code;
	std::string name;
	std::string description;
	uint32_t usage_count;
	snowflake creator_id;
	time_t created_at;
	time_t updated_at;
	snowflake source_guild_id;
	bool is_dirty;

	dtemplate();
	~dtemplate();
	dtemplate& fill_from_json(nlohmann::json* j);
	std::string build_json() const;

};

/** A container of invites */
typedef std::unordered_map<snowflake, dtemplate> dtemplate_map;


};
