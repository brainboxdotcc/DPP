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
#include <dpp/voiceregion.h>
#include <dpp/discordevents.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

voiceregion::voiceregion() : flags(0) 
{
}

voiceregion::~voiceregion() = default;

voiceregion& voiceregion::fill_from_json(nlohmann::json* j) {
	id = string_not_null(j, "id");
	name = string_not_null(j, "id");
	if (bool_not_null(j, "optimal"))
		flags |= v_optimal;
	if (bool_not_null(j, "deprecated"))
		flags |= v_deprecated;
	if (bool_not_null(j, "custom"))
		flags |= v_custom;
	if (bool_not_null(j, "vip"))
		flags |= v_vip;
	return *this;
}

std::string voiceregion::build_json() const {
	return json({
		{ "id", id },
		{ "name", name },
		{ "optimal", is_optimal() },
		{ "deprecated", is_deprecated() },
		{ "custom", is_custom() },
		{ "vip", is_vip() }
	}).dump();
}

bool voiceregion::is_optimal() const {
	return flags & v_optimal;
}

bool voiceregion::is_deprecated() const {
	return flags & v_deprecated;
}

bool voiceregion::is_custom() const {
	return flags & v_custom;
}

bool voiceregion::is_vip() const {
	return flags & v_vip;
}


};

