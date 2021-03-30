#include <dpp/discord.h>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

role::role() :
	managed(),
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

void role::fill_from_json(nlohmann::json* j)
{
	this->id = SnowflakeNotNull(j, "id");
	this->colour = Int32NotNull(j, "color");
	this->position = Int8NotNull(j, "position");
	this->permissions = Int32NotNull(j, "permissions");
	this->flags |= BoolNotNull(j, "hoist") ? dpp::r_hoist : 0;
	this->flags |= BoolNotNull(j, "managed") ? dpp::r_managed : 0;
	this->flags |= BoolNotNull(j, "mentionable") ? dpp::r_mentionable : 0;
	if (j->find("tags") != j->end()) {
		auto t = (*j)["tags"];
		this->flags |= BoolNotNull(&t, "premium_subscriber") ? dpp::r_premium_subscriber : 0;
		this->bot_id = SnowflakeNotNull(&t, "bot_id");
		this->integration_id = SnowflakeNotNull(&t, "integration_id");
	}
}

};
