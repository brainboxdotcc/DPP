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

	enum image_type {
		i_png,
		i_jpg,
		i_gif
	};

	enum loglevel {
		ll_trace = 0,
		ll_debug,
		ll_info,
		ll_warning,
		ll_error,
		ll_critical
	};

	namespace utility {
		std::string current_date_time();
		std::string loglevel(dpp::loglevel in);
	};

};

#include <dpp/role.h>
#include <dpp/user.h>
#include <dpp/channel.h>
#include <dpp/guild.h>
#include <dpp/invite.h>
#include <dpp/dtemplate.h>
#include <dpp/emoji.h>
#include <dpp/ban.h>
#include <dpp/prune.h>
#include <dpp/voiceregion.h>
#include <dpp/integration.h>
#include <dpp/webhook.h>
#include <dpp/voicestate.h>
#include <dpp/presence.h>
#include <dpp/intents.h>
