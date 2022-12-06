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
#include <dpp/discordevents.h>
#include <dpp/stringops.h>
#include <dpp/integration.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/cache.h>

using json = nlohmann::json;

namespace dpp {

integration::integration() :
	managed(),
	type(i_twitch),
	flags(0),
	role_id(0),
	user_id(0),
	expire_grace_period(0),
	synced_at(0),
	subscriber_count(0)
{
	app.id = 0;
	app.bot = nullptr;
}

integration::~integration()
{
}

integration& integration::fill_from_json(nlohmann::json* j)
{
	std::map<std::string, integration_type> type_map = {
		{ "", i_discord },
		{ "youtube", i_youtube },
		{ "twitch", i_twitch },
		{ "discord", i_discord }
	};
	this->id = snowflake_not_null(j, "id");
	this->name = string_not_null(j, "name");
	this->type = type_map[string_not_null(j, "type")];
	if (bool_not_null(j, "enabled"))
		this->flags |= if_enabled;
	if (bool_not_null(j, "syncing"))
		this->flags |= if_syncing;
	if (bool_not_null(j, "enable_emoticons"))
		this->flags |= if_emoticons;
	if (bool_not_null(j, "revoked"))
		this->flags |= if_revoked;
	if (int8_not_null(j, "expire_behavior"))
		this->flags |= if_expire_kick;
	this->expire_grace_period = int32_not_null(j, "expire_grace_period");
	if (j->contains("user")) {
		auto t = (*j)["user"];
		this->user_id = snowflake_not_null(&t, "user_id");
	}
	if (j->contains("application")) {
		auto & t = (*j)["application"];
		this->app.id = snowflake_not_null(&t, "id");
		if (t.find("bot") != t.end()) {
			auto & b = t["bot"];
			this->app.bot = dpp::find_user(snowflake_not_null(&b, "id"));
		}
	}
	this->subscriber_count = int32_not_null(j, "subscriber_count");

	this->account_id = string_not_null(&((*j)["account"]), "id");
	this->account_name = string_not_null(&((*j)["account"]), "name");

	return *this;
}

std::string integration::build_json([[maybe_unused]] bool with_id) const {
	return json({
		{ "expire_behavior", (flags & if_expire_kick) ? 1 : 0 },
		{ "expire_grace_period", expire_grace_period },
		{ "enable_emoticons", emoticons_enabled() }
	}).dump();
}

bool integration::emoticons_enabled() const {
	return flags & if_emoticons;
}

bool integration::is_enabled() const {
	return flags & if_enabled;
}

bool integration::is_syncing() const {
	return flags & if_syncing;
}

bool integration::is_revoked() const {
	return flags & if_revoked;
}

bool integration::expiry_kicks_user() const {
	return flags & if_expire_kick;
}

connection::connection() : id({}), revoked(false), verified(false), friend_sync(false), show_activity(false), visible(false) {
}

connection& connection::fill_from_json(nlohmann::json* j) {
	this->id = string_not_null(j, "id");
	this->name = string_not_null(j, "name");
	this->type = string_not_null(j, "type");
	this->revoked = bool_not_null(j, "revoked");
	this->verified = bool_not_null(j, "verified");
	this->friend_sync = bool_not_null(j, "friend_sync");
	this->show_activity = bool_not_null(j, "show_activity");
	this->two_way_link = bool_not_null(j, "two_way_link");
	this->visible = (int32_not_null(j, "visibility") == 1);
	if (j->contains("integrations")) {
		integrations.reserve((*j)["integrations"].size());
		for (auto & i : (*j)["integrations"]) {
			integrations.emplace_back(integration().fill_from_json(&i));
		}
	}
	return *this;
}


};
