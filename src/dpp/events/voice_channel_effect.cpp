/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include "voice_channel_effect.h"


#include <dpp/discordevents.h>
#include <dpp/json.h>
#include <dpp/voicestate.h>

#include <iostream>

namespace dpp {

using json = nlohmann::json;

voice_channel_effect::voice_channel_effect() : shard_id(0), channel_id(0), guild_id(0), user_id(0), animation_type(0), animation_id(0), sound_id(0), sound_volume(0)
{
}

voice_channel_effect& voice_channel_effect::fill_from_json_impl(nlohmann::json* j) {
	set_snowflake_not_null(j, "channel_id", channel_id);
	set_snowflake_not_null(j, "guild_id", guild_id);
	set_snowflake_not_null(j, "user_id", user_id);
	emoji.fill_from_json(&j->at("emoji"));
	if (int8_not_null(j, "animation_type")) {
		set_int8_not_null(j, "animation_type", animation_type);
	}
	if (int8_not_null(j, "animation_id")) {
		set_int32_not_null(j, "animation_id", animation_id);
	}
	if (int8_not_null(j, "sound_id")) {
		set_snowflake_not_null(j, "sound_id", sound_id);
	}
	if (int8_not_null(j, "sound_volume")) {
		set_double_not_null(j, "sound_volume", sound_volume);
	}
	return *this;
}

}
