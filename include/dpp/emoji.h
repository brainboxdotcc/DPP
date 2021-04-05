#pragma once
#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

enum emoji_flags {
	e_require_colons = 0b00000001,
	e_managed =        0b00000010,
	e_animated =       0b00000100,
	e_available =      0b00001000,
};

class emoji : public managed {
public:
	std::string name;
	snowflake user_id;
	uint8_t flags;
	
	emoji();
	~emoji();
	emoji& fill_from_json(nlohmann::json* j);
	std::string build_json(bool with_id = false) const;

	bool requires_colons() const;
	bool is_managed() const;
	bool is_animated() const;
	bool is_available() const;
};

typedef std::unordered_map<snowflake, emoji> emoji_map;

};
