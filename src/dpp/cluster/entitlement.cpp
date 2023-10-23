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
#include <dpp/restrequest.h>
#include <dpp/entitlement.h>

namespace dpp {

void cluster::list_entitlements(snowflake guild_id, command_completion_event_t callback) {
	rest_request_list<entitlement>(this, API_PATH "/applications", me.id.str(), "entitlements", m_get, "", callback);
}

void cluster::create_test_entitlement(const class entitlement& new_entitlement, command_completion_event_t callback) {
	json j;
	j["sku_id"] = new_entitlement.sku_id.str();
	j["owner_id"] = new_entitlement.owner_id.str();
	j["owner_type"] = new_entitlement.type;
	rest_request<entitlement>(this, API_PATH "/applications", me.id.str(), "entitlements", m_post, j, callback);
}

void cluster::delete_test_entitlement(const class snowflake entitlement_id, command_completion_event_t callback) {
	rest_request<confirmation>(this, API_PATH "/applications", me.id.str(), "entitlements/" + entitlement_id.str(), m_delete, "", callback);
}

} // namespace dpp
