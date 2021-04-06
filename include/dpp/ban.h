#pragma once
#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

class ban {
public:
	std::string reason;
	snowflake user_id;
	
	ban();
	~ban();
	ban& fill_from_json(nlohmann::json* j);
	std::string build_json() const;
};

typedef std::unordered_map<snowflake, ban> ban_map;

};
