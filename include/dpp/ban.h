#pragma once
#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

/** The ban class represents a ban on a guild.
 */
class ban {
public:
	/** The ban reason */
	std::string reason;
	/** User ID the ban applies to */
	snowflake user_id;
	
	/** Constructor */
	ban();

	/** Destructor */
	~ban();

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	ban& fill_from_json(nlohmann::json* j);
	std::string build_json() const;
};

/** A group of bans
 */
typedef std::unordered_map<snowflake, ban> ban_map;

};
