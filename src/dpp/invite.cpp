#include <dpp/invite.h>
#include <dpp/discordevents.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

invite::invite() : guild_id(0), channel_id(0), inviter_id(0), target_user_id(0), target_user_type(1), approximate_presence_count(0), approximate_member_count(0)
{
}

invite::~invite() {
}


invite& invite::fill_from_json(nlohmann::json* j) {
	code = StringNotNull(j, "code");
	guild_id = (j->find("guild") != j->end()) ? SnowflakeNotNull(&((*j)["guild_id"]), "id") : 0;
	channel_id = (j->find("channel") != j->end()) ? SnowflakeNotNull(&((*j)["channel"]), "id") : 0;
	inviter_id = (j->find("inviter") != j->end()) ? SnowflakeNotNull(&((*j)["inviter"]), "id") : 0;
	target_user_id = (j->find("target_user") != j->end()) ? SnowflakeNotNull(&((*j)["target_user"]), "id") : 0;
	target_user_type = Int8NotNull(j, "target_user_type");
	approximate_presence_count = Int32NotNull(j, "approximate_presence_count");
	approximate_member_count = Int32NotNull(j, "approximate_member_count");
	return *this;
}

std::string invite::build_json() const {
	json j({
		{"code", code},
		{"guild_id", guild_id},
		{"channel_id", channel_id},
		{"inviter_id", inviter_id},
		{"target_user_id", target_user_id},
		{"target_user_type", target_user_type},
		{"approximate_presence_count", approximate_presence_count},
		{"approximate_member_count", approximate_member_count}
	});
	return j.dump();
}

};
