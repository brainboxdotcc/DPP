#include <dpp/dtemplate.h>
#include <dpp/discordevents.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

dtemplate::dtemplate() : code(""), name(""), description(""), usage_count(0), creator_id(0), source_guild_id(0)
{
}

dtemplate::~dtemplate() {
}


dtemplate& dtemplate::fill_from_json(nlohmann::json* j) {
	code = StringNotNull(j, "code");
	name = StringNotNull(j, "name");
	description = StringNotNull(j, "description");
	usage_count = Int32NotNull(j, "usage_count");
	creator_id = SnowflakeNotNull(j, "creator_id");
	created_at = TimestampNotNull(j, "created_at");
	updated_at = TimestampNotNull(j, "updated_at");
	source_guild_id = SnowflakeNotNull(j, "source_guild_id");
	is_dirty = BoolNotNull(j, "is_dirty");
	return *this;
}

std::string dtemplate::build_json() const {
	json j({
		{"code", code},
		{"name", name},
		{"description", description},
		{"usage_count", usage_count},
		{"creator_id", creator_id},
		{"updated_at", updated_at},
		{"source_guild_id", source_guild_id,
		"is_dirty", is_dirty}
	});
	return j.dump();
}

};
