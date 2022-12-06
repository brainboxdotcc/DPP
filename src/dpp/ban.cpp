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
#include <dpp/ban.h>
#include <dpp/discordevents.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

ban::ban() : user_id(0)
{
}

ban& ban::fill_from_json(nlohmann::json* j) {
	reason = string_not_null(j, "reason");
	if (j->contains("user")) {
		json & user = (*j)["user"];
		user_id = snowflake_not_null(&user, "id");
	}
	return *this;
}

std::string ban::build_json([[maybe_unused]] bool with_id) const {
	/* This is an unused stub, because sending a ban is simple as a user id and a reason */
	return "{}";
}

};

