#include <dpp/ban.h>
#include <dpp/discordevents.h>
#include <dpp/discord.h>
#include <nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

ban::ban() : user_id(0)
{
}

ban::~ban() {
}

ban& ban::fill_from_json(nlohmann::json* j) {
	reason = StringNotNull(j, "reason");
	if (j->find("user") != j->end()) {
		json & user = (*j)["user"];
		user_id = SnowflakeNotNull(&user, "id");
	}
	return *this;
}

std::string ban::build_json() const {
	return "{}";
}

};

