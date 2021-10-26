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
#include <dpp/stage_instance.h>

namespace dpp {

using json = nlohmann::json;

stage_instance::stage_instance() :
	id(0),
	guild_id(0),
	channel_id(0),	
	privacy_level(sp_public),
	discoverable_disabled(false)
{
}

stage_instance& stage_instance::fill_from_json(const json* j) {
	SetSnowflakeNotNull(j, "id", this->id);
	SetSnowflakeNotNull(j, "guild_id", this->guild_id);
	SetSnowflakeNotNull(j, "channel_id", this->channel_id);
	SetStringNotNull(j, "topic", this->topic) ;
	SetInt8NotNull(j, "privacy_level", this->privacy_level);
	SetBoolNotNull(j, "discoverable_disabled", this->discoverable_disabled);

	return *this;
}

std::string const stage_instance::build_json() const {
	json j;
	j["topic"] = this->topic;
	j["privacy_level"] = this->privacy_level;
	j["channel_id"] = this->channel_id;

	return j.dump();
}

};
