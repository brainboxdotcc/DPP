#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>

namespace dpp {
	/** A 64 bit unsigned value representing many things on discord.
	 * Discord calls the value a 'snowflake' value.
	 */
	typedef uint64_t snowflake;

	/** The managed class is the base class for various types that can
	 * be stored in a cache that are identified by a dpp::snowflake id
	 */
	class managed {
	public:
		/** Unique ID of object */
		snowflake id;
		/** Constructor, initialises id to 0 */
		managed();
		/** Default destructor */
		~managed() = default;
	};

};

#include <dpp/role.h>
#include <dpp/user.h>
#include <dpp/channel.h>
#include <dpp/guild.h>
#include <dpp/invite.h>
#include <dpp/dtemplate.h>
#include <dpp/emoji.h>
#include <dpp/intents.h>
