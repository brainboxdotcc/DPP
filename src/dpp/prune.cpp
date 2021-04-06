#include <dpp/prune.h>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>
#include <nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

prune& prune::fill_from_json(nlohmann::json* j) {
	days = Int32NotNull(j, "days");
	compute_prune_count = BoolNotNull(j, "compute_prune_count");
	if (j->find("include_roles") != j->end()) {
		for (auto & r : (*j)["include_roles"]) {
			include_roles.push_back(from_string<uint64_t>(r.get<std::string>(), std::dec));
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
