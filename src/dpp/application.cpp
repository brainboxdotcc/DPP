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
#include <dpp/discord.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

application::application() : id(0), bot_public(false), bot_require_code_grant(false), guild_id(0), primary_sku_id(0), flags(0)
{
}

application::~application() = default;

application& application::fill_from_json(nlohmann::json* j) {
	SetSnowflakeNotNull(j, "id", id);
	SetStringNotNull(j, "name", name);
	std::string ic = StringNotNull(j, "icon");
	if (!ic.empty()) {
		icon = ic;
	}
	SetStringNotNull(j, "description", description);
	SetStringNotNull(j, "rpc_origins", rpc_origins);
	SetBoolNotNull(j, "bot_public", bot_public);
	SetBoolNotNull(j, "bot_require_code_grant", bot_require_code_grant);
	SetStringNotNull(j, "terms_of_service_url", terms_of_service_url);
	SetStringNotNull(j, "privacy_policy_url", privacy_policy_url);
	owner = user().fill_from_json(&((*j)["owner"]));
	SetStringNotNull(j, "summary", summary);
	SetStringNotNull(j, "verify_key", verify_key);
	SetSnowflakeNotNull(j, "guild_id", guild_id);
	SetSnowflakeNotNull(j, "primary_sku_id", primary_sku_id);
	SetStringNotNull(j, "slug", slug);
	std::string ci = StringNotNull(j, "cover_image");
	if (!ci.empty()) {
		cover_image = ci;
	}
	SetInt32NotNull(j, "flags", flags);
	if (j->at("team")) {
		json& t = (*j)["team"];
		std::string i = StringNotNull(&t, "icon");
		if (!i.empty()) {
			icon = i;
		}
		SetSnowflakeNotNull(&t, "id", this->team.id);
		SetStringNotNull(&t, "name", this->team.name);
		SetSnowflakeNotNull(&t, "owner_user_id", this->team.owner_user_id);
		for (auto m : t["members"]) {
			team_member tm;
			tm.membership_state = (team_member_status)Int32NotNull(&m, "membership_state");
			SetStringNotNull(&m, "permissions", tm.permissions);
			SetSnowflakeNotNull(&m, "team_id", tm.team_id);
			tm.member_user = user().fill_from_json(&m["user"]);
			this->team.members.emplace_back(tm);
		}
	}
	return *this;
}

};

