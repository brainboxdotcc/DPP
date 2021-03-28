#include <dpp/discord.h>

namespace dpp {

channel::channel() :
	id(0),
	flags(0),
	guild_id(0),
	position(0),
	last_message_id(0),
	user_limit(0),
	rate_limit_per_user(0),
	owner_id(0),
	parent_id(0),
	last_pin_timestamp(0)
{
}

channel::~channel()
{
}

};
