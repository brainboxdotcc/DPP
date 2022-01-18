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
#include <dpp/invite.h>
#include <dpp/discordevents.h>
#include <dpp/nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

invite::invite() : expires_at(0), guild_id(0), channel_id(0), inviter_id(0), target_user_id(0), target_user_type(1), approximate_presence_count(0), approximate_member_count(0), uses(0)
{
}

invite::~invite() = default;


invite& invite::fill_from_json(nlohmann::json* j) {
	code = string_not_null(j, "code");
	expires_at = (j->find("expires_at") != j->end()) ? ts_not_null(j, "expires_at") : 0;
	guild_id = (j->find("guild") != j->end()) ? snowflake_not_null(&((*j)["guild"]), "id") : 0;
	channel_id = (j->find("channel") != j->end()) ? snowflake_not_null(&((*j)["channel"]), "id") : 0;
	inviter_id = (j->find("inviter") != j->end()) ? snowflake_not_null(&((*j)["inviter"]), "id") : 0;
	target_user_id = (j->find("target_user") != j->end()) ? snowflake_not_null(&((*j)["target_user"]), "id") : 0;
	target_user_type = int8_not_null(j, "target_user_type");
	approximate_presence_count = int32_not_null(j, "approximate_presence_count");
	approximate_member_count = int32_not_null(j, "approximate_member_count");
	max_age = int32_not_null(j, "max_age");
	max_uses = int32_not_null(j, "max_uses");
	temporary = bool_not_null(j, "temporary");
	unique = bool_not_null(j, "unique");
	uses = (j->find("uses") != j->end()) ? int32_not_null(j, "uses") : 0;
	return *this;
}

std::string invite::build_json() const {
	json j;
	if (max_age > 0)
		j["max_age"] = max_age;
	if (max_uses > 0)
		j["max_uses"] = max_uses;
	if (target_user_id > 0)
		j["target_user"] = target_user_id;
	if (target_user_type > 0)
		j["target_user_type"] = target_user_type;
	if (temporary)
		j["temporary"] = temporary;
	if (unique)
		j["unique"] = unique;
	return j.dump();
}

};
