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
#include <dpp/prune.h>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

prune& prune::fill_from_json(nlohmann::json* j) {
	days = int32_not_null(j, "days");
	compute_prune_count = bool_not_null(j, "compute_prune_count");
	if (j->find("include_roles") != j->end()) {
		for (auto & r : (*j)["include_roles"]) {
			include_roles.push_back(from_string<uint64_t>(r.get<std::string>()));
		}
	}
	return *this;
}

std::string prune::build_json(bool with_prune_count) const {
	json j;
	for (auto & r : include_roles) {
		j["include_roles"].push_back(std::to_string(r));
	}
	if (with_prune_count) {
		j["compute_prune_count"] = compute_prune_count;
	}
	j["days"] = days;
	return j.dump();
}

};
