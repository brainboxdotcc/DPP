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
#pragma once
#include "emoji.h"
#include "voicestate.h"


#include <dpp/export.h>
#include <dpp/json_fwd.h>
#include <dpp/json_interface.h>
#include <dpp/snowflake.h>
#include <unordered_map>

namespace dpp {

/**
 * @brief Animation types that a voice channel effect can have
 */
enum animation_types : uint8_t {
	at_premium = 0,
	at_basic = 1
};

/**
 * @brief Represents the voice state of a user on a guild
 * These are stored in the dpp::guild object, and accessible there,
 * or via dpp::channel::get_voice_members
 */
class DPP_EXPORT voice_channel_effect : public json_interface<voice_channel_effect> {
protected:
	friend struct json_interface<voice_channel_effect>;

	/**
	 * @brief Fill voice channel effect object from json data
	 *
	 * @param j JSON data to fill from
	 * @return voice_channel_effect& Reference to self
	 */
	voice_channel_effect& fill_from_json_impl(nlohmann::json* j);

public:
	/**
	 * @brief Owning shard.
	 */
	int32_t shard_id{0};

	/**
	 * @brief The channel id this user is connected to.
	 *
	 * @note This may be empty.
	 */
	snowflake channel_id{0};

	/**
	 *
	 * @brief Optional: The guild id this voice state is for.
	 */
	snowflake guild_id{0};

	/**
	 * @brief The user id this voice state is for.
	 */
	snowflake user_id{0};

	/**
	 * @brief The emoji of the voice channel effect.
	 */
	dpp::emoji emoji;

	/**
	 * @brief The type of emoji animation, for emoji reaction and soundboard effects.
	 */
	uint8_t animation_type{0};

	/**
	 * @brief The ID of the emoji animation, for emoji reaction and soundboard effects.
	 */
	uint32_t animation_id{0};

	/**
	 * @brief The ID of the soundboard sound, for soundboard effects.
	 */
	snowflake sound_id{0};

	/**
	 * @brief The volume of the soundboard sound, from 0 to 1, for soundboard effects.
	 */
	double sound_volume{0};

	/**
	 * @brief Construct a new voice_channel_effect object
	 */
	voice_channel_effect();

	/**
	 * @brief Destroy the voicestate object
	 */
	virtual ~voice_channel_effect() = default;
};

}