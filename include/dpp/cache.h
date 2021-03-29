#pragma once

#include <dpp/discord.h>

namespace dpp {

	void garbage_collection();
	void store_guild(guild* g);
	guild* find_guild(snowflake id);
	void store_user(user * u);
	user* find_user(snowflake id);
	void store_channel(channel* c);
	channel* find_channel(snowflake id);
	void store_role(role* r);
	role* find_role(snowflake id);
};

