#include <dpp/emoji.h>
#include <dpp/discordevents.h>
#include <dpp/discord.h>
#include <nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

emoji::emoji() : managed(), user_id(0), flags(0)
{
}

emoji::~emoji() {
}

emoji& emoji::fill_from_json(nlohmann::json* j) {
	id = SnowflakeNotNull(j, "id");
	name = StringNotNull(j, "name");
	if (j->find("user") != j->end()) {
		json & user = (*j)["user"];
		user_id = SnowflakeNotNull(&user, "id");
	}
	if (BoolNotNull(j, "require_colons"))
		flags |= e_require_colons;
	if (BoolNotNull(j, "managed"))
		flags |= e_managed;
	if (BoolNotNull(j, "animated"))
		flags |= e_animated;
	if (BoolNotNull(j, "available"))
		flags |= e_available;
	return *this;
}

std::string emoji::build_json(bool with_id) const {
	json j;
	if (with_id) {
		j["id"] = std::to_string(id);
	}
	if (flags & e_require_colons) {
		j["require_colons"] = true;
	}
	if (flags & e_managed) {
		j["managed"] = true;
	}
	if (flags & e_animated) {
		j["animated"] = true;
	}
	if (flags & e_available) {
		j["available"] = true;
	}
	return j.dump();
}

bool emoji::requires_colons() const {
	return flags & e_require_colons;
}

bool emoji::is_managed() const {
	return flags & e_managed;
}

bool emoji::is_animated() const {
	return flags & e_animated;
}

bool emoji::is_available() const {
	return flags & e_available;
}


};

