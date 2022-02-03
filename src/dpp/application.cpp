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
#include <dpp/application.h>
#include <dpp/discordevents.h>
#include <dpp/snowflake.h>
#include <dpp/managed.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

application::application() : managed(0), bot_public(false), bot_require_code_grant(false), guild_id(0), primary_sku_id(0), flags(0)
{
}

application::~application() = default;

application& application::fill_from_json(nlohmann::json* j) {
	set_snowflake_not_null(j, "id", id);
	set_string_not_null(j, "name", name);
	std::string ic = string_not_null(j, "icon");
	if (!ic.empty()) {
		icon = ic;
	}
	set_string_not_null(j, "description", description);
	set_string_not_null(j, "rpc_origins", rpc_origins);
	set_bool_not_null(j, "bot_public", bot_public);
	set_bool_not_null(j, "bot_require_code_grant", bot_require_code_grant);
	set_string_not_null(j, "terms_of_service_url", terms_of_service_url);
	set_string_not_null(j, "privacy_policy_url", privacy_policy_url);
	owner = user().fill_from_json(&((*j)["owner"]));
	set_string_not_null(j, "summary", summary);
	set_string_not_null(j, "verify_key", verify_key);
	set_snowflake_not_null(j, "guild_id", guild_id);
	set_snowflake_not_null(j, "primary_sku_id", primary_sku_id);
	set_string_not_null(j, "slug", slug);
	std::string ci = string_not_null(j, "cover_image");
	if (!ci.empty()) {
		cover_image = ci;
	}
	set_int32_not_null(j, "flags", flags);
	if (j->at("team")) {
		json& t = (*j)["team"];
		std::string i = string_not_null(&t, "icon");
		if (!i.empty()) {
			icon = i;
		}
		set_snowflake_not_null(&t, "id", this->team.id);
		set_string_not_null(&t, "name", this->team.name);
		set_snowflake_not_null(&t, "owner_user_id", this->team.owner_user_id);
		for (auto m : t["members"]) {
			team_member tm;
			tm.membership_state = (team_member_status)int32_not_null(&m, "membership_state");
			set_string_not_null(&m, "permissions", tm.permissions);
			set_snowflake_not_null(&m, "team_id", tm.team_id);
			tm.member_user = user().fill_from_json(&m["user"]);
			this->team.members.emplace_back(tm);
		}
	}
	return *this;
}

};

