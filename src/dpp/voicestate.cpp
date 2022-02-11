/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2021 Craig Edwards and D++ contributors 
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/
#include <dpp/voicestate.h>
#include <dpp/discordevents.h>
#include <dpp/nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

voicestate::voicestate() : shard(nullptr), guild_id(0), channel_id(0), user_id(0)
{
}

voicestate::~voicestate() = default;

voicestate& voicestate::fill_from_json(nlohmann::json* j) {
	guild_id = snowflake_not_null(j, "guild_id");
	channel_id = snowflake_not_null(j, "channel_id");
	user_id = snowflake_not_null(j, "user_id");
	session_id = string_not_null(j, "session_id");
	flags = 0;
	if (bool_not_null(j, "deaf"))
		flags |= vs_deaf;
	if (bool_not_null(j, "mute"))
		flags |= vs_mute;
	if (bool_not_null(j, "self_mute"))
		flags |= vs_self_mute;
	if (bool_not_null(j, "self_deaf"))
		flags |= vs_self_deaf;
	if (bool_not_null(j, "self_stream"))
		flags |= vs_self_stream;
	if (bool_not_null(j, "self_video"))
		flags |= vs_self_video;
	if (bool_not_null(j, "suppress"))
		flags |= vs_suppress;
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

bool voicestate::is_suppressed() const {
	return flags & vs_suppress;
}

std::string voicestate::build_json() const {
	return json({}).dump();
}

};
