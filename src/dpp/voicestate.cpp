#include <dpp/voicestate.h>
#include <dpp/discordevents.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

voicestate::voicestate() : guild_id(0), channel_id(0), user_id(0)
{
}

voicestate::~voicestate() {
}

voicestate& voicestate::fill_from_json(nlohmann::json* j) {
	guild_id = SnowflakeNotNull(j, "guild_id");
	channel_id = SnowflakeNotNull(j, "channel_id");
	user_id = SnowflakeNotNull(j, "user_id");
	session_id = StringNotNull(j, "session_id");
	uint8_t flags = 0;
	if (BoolNotNull(j, "deaf"))
		flags |= vs_deaf;
	if (BoolNotNull(j, "mute"))
		flags |= vs_mute;
	if (BoolNotNull(j, "self_mute"))
		flags |= vs_self_mute;
	if (BoolNotNull(j, "self_deaf"))
		flags |= vs_self_deaf;
	if (BoolNotNull(j, "self_stream"))
		flags |= vs_self_stream;
	if (BoolNotNull(j, "self_video"))
		flags |= vs_self_video;
	if (BoolNotNull(j, "supress"))
		flags |= vs_supress;
	return *this;
}

bool voicestate::is_deaf() const {
	return flags & vs_deaf;
}

bool voicestate::is_mute() const {
	return flags & vs_mute;
}

bool voicestate::is_self_mute() const {
	return flags & vs_self_mute;
}

bool voicestate::is_self_deaf() const {
	return flags & vs_self_deaf;
}

bool voicestate::self_stream() const {
	return flags & vs_self_stream;
}

bool voicestate::self_video() const {
	return flags & vs_self_video;
}

bool voicestate::is_supressed() const {
	return flags & vs_supress;
}

std::string voicestate::build_json() const {
	return json({}).dump();
}

};
