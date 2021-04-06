#include <dpp/voiceregion.h>
#include <dpp/discordevents.h>
#include <dpp/discord.h>
#include <nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

voiceregion::voiceregion() : flags(0) 
{
}

voiceregion::~voiceregion() {
}

voiceregion& voiceregion::fill_from_json(nlohmann::json* j) {
	id = StringNotNull(j, "id");
	name = StringNotNull(j, "id");
	if (BoolNotNull(j, "optimal"))
		flags |= v_optimal;
	if (BoolNotNull(j, "deprecated"))
		flags |= v_deprecated;
	if (BoolNotNull(j, "custom"))
		flags |= v_custom;
	if (BoolNotNull(j, "vip"))
		flags |= v_vip;
	return *this;
}

std::string voiceregion::build_json() const {
	return json({
		{ "id", id },
		{ "name", name },
		{ "optimal", is_optimal() },
		{ "deprecated", is_deprecated() },
		{ "custom", is_custom() },
		{ "vip", is_vip() }
	}).dump();
}

bool voiceregion::is_optimal() const {
	return flags & v_optimal;
}

bool voiceregion::is_deprecated() const {
	return flags & v_deprecated;
}

bool voiceregion::is_custom() const {
	return flags & v_custom;
}

bool voiceregion::is_vip() const {
	return flags & v_vip;
}


};

