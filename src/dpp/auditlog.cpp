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
#include <dpp/auditlog.h>
#include <dpp/discordevents.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

auditlog::auditlog() = default;

auditlog::~auditlog() = default;

auditlog& auditlog::fill_from_json(nlohmann::json* j) {
	for (auto & ai : (*j)["audit_log_entries"]) {
		audit_entry ae;
		ae.id = snowflake_not_null(&ai, "id");
		ae.event = (audit_type)int8_not_null(&ai, "action_type");
		ae.user_id = snowflake_not_null(&ai, "user_id");
		ae.target_id = snowflake_not_null(&ai, "target_id");
		ae.reason = string_not_null(&ai, "reason");
		if (j->find("changes") != j->end()) {
			auto &c = ai["changes"];
			for (auto & change : c) {
				audit_change ac;
				ac.key = string_not_null(&change, "key");
				if (change.find("new_value") != change.end()) {
					ac.new_value = change["new_value"].dump();
				}
				if (change.find("old_value") != change.end()) {
					ac.old_value = change["old_value"].dump();
				}
			}
		}
		if (j->find("options") != j->end()) {
			auto &o = ai["options"];
			audit_extra opts;
			opts.channel_id = snowflake_not_null(&o, "channel_id");
			opts.count = string_not_null(&o, "count");
			opts.delete_member_days = string_not_null(&o, "delete_member_days");
			opts.id = snowflake_not_null(&o, "id");
			opts.members_removed = string_not_null(&o, "members_removed");
			opts.message_id = snowflake_not_null(&o, "message_id");
			opts.role_name = string_not_null(&o, "role_name");
			opts.type = string_not_null(&o, "type");
			ae.options = opts;
		}
		this->entries.emplace_back(ae);
	}
	return *this;
}

};

