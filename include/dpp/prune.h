#pragma once
#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

struct prune {
	uint32_t days = 0;
	std::vector<snowflake> include_roles;
	bool compute_prune_count;

	prune& fill_from_json(nlohmann::json* j);
	std::string build_json(bool with_prune_count) const;

};

};
