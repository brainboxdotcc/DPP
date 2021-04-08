#pragma once
#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

/** Defines a request to count prunable users, or start a prune operation
 */
struct prune {
	/** Number of days to include in the prune
	 */
	uint32_t days = 0;
	/** Roles to include in the prune (empty to include everyone)
	 */
	std::vector<snowflake> include_roles;
	/** True if the count of pruneable users should be returned
	 * (discord recommend not using this on big guilds)
	 */
	bool compute_prune_count;

	/** Fill this object from json.
	 * @param j JSON object to fill from
	 * @return A reference to self
	 */
	prune& fill_from_json(nlohmann::json* j);

	/** Build JSON from this object.
	 * @param with_prune_count True if the prune count boolean is to be set in the built JSON
	 * @return The JSON text of the prune object
	 */
	std::string build_json(bool with_prune_count) const;

};

};
