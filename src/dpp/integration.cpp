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
#include <dpp/discord.h>
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
	this->id = SnowflakeNotNull(j, "id");
	this->name = StringNotNull(j, "name");
	this->type = type_map[StringNotNull(j, "type")];
	if (BoolNotNull(j, "enabled"))
		this->flags |= if_enabled;
	if (BoolNotNull(j, "syncing"))
		this->flags |= if_syncing;
	if (BoolNotNull(j, "enable_emoticons"))
		this->flags |= if_emoticons;
	if (BoolNotNull(j, "revoked"))
		this->flags |= if_revoked;
	if (Int8NotNull(j, "expire_behavior"))
		this->flags |= if_expire_kick;
	this->expire_grace_period = Int32NotNull(j, "expire_grace_period");
	if (j->find("user") != j->end()) {
		auto t = (*j)["user"];
		this->user_id = SnowflakeNotNull(&t, "user_id");
	}
	if (j->find("application") != j->end()) {
		auto & t = (*j)["application"];
		this->app.id = SnowflakeNotNull(&t, "id");
		if (t.find("bot") != t.end()) {
			auto & b = t["bot"];
			this->app.bot = dpp::find_user(SnowflakeNotNull(&b, "id"));
		}
	}
	this->subscriber_count = Int32NotNull(j, "subscriber_count");

	this->account_id = StringNotNull(&((*j)["account"]), "id");
	this->account_name = StringNotNull(&((*j)["account"]), "name");

	return *this;
}

std::string integration::build_json() const {
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

connection::connection() : id(0), revoked(false), verified(false), friend_sync(false), show_activity(false), visible(false) {
}

connection& connection::fill_from_json(nlohmann::json* j) {
	this->id = SnowflakeNotNull(j, "id");
	this->name = StringNotNull(j, "name");
	this->type = StringNotNull(j, "type");
	this->revoked = BoolNotNull(j, "revoked");
	this->verified = BoolNotNull(j, "verified");
	this->friend_sync = BoolNotNull(j, "friend_sync");
	this->show_activity = BoolNotNull(j, "show_activity");
	this->visible = (Int32NotNull(j, "visibility") == 1);
	if (j->find("integrations") != j->end()) {
		for (auto & i : (*j)["integrations"]) {
			integrations.push_back(integration().fill_from_json(&i));
		}
	}
	return *this;
}


};
