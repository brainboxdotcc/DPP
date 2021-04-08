#pragma once

#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

/** Represents an invite to a discord guild or channel
 */
class invite {
public:
	/** Invite code
	 */
	std::string code;
	/** Guild for the invite
	 */
	snowflake guild_id;
	/** Channel id for invite 
	 */
	snowflake channel_id;
	/** User ID of invite creator
	 */
	snowflake inviter_id;
	/** Target user ID of invite, for invites sent via DM
	 */
	snowflake target_user_id;
	/** Target user type (generally this is always 1, "stream")
	 */
	uint8_t target_user_type;
	/** Approximate number of online users
	 */
	uint32_t approximate_presence_count;
	/** Approximate total users online and offline
	 */
	uint32_t approximate_member_count;
	/** Maximum age of invite
	 */
	uint32_t max_age;
	/** Maximum number of uses
	 */
	uint32_t max_uses;
	/** True if a temporary invite which grants access for a limited time
	 */
	bool temporary;
	/** True if this invite should not replace or "attach to" similar invites
	 */
	bool unique;

	/** Constructor
	 */
	invite();

	/** Destructor
	 */
	~invite();

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	invite& fill_from_json(nlohmann::json* j);

	/** Build JSON from this object.
	 * @return The JSON text of the invite
	 */
	std::string build_json() const;

};

/** A container of invites */
typedef std::unordered_map<std::string, invite> invite_map;

};
