#include <dpp/discord.h>

namespace dpp {

role::role() :
	id(0),
	colour(0),
	position(0),
	permissions(0),
	flags(0),
	integration_id(0),
	bot_id(0)
{
}

role::~role()
{
}

};
