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
#include <dpp/discordvoiceclient.h>
#include <dpp/cluster.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

void cluster::get_voice_regions(command_completion_event_t callback) {
	this->post_rest("/voice/v9/regions", "", "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		voiceregion_map voiceregions;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_region : j) {
				voiceregions[string_not_null(&curr_region, "id")] = voiceregion().fill_from_json(&j);
			}
		}
		callback(confirmation_callback_t("voiceregion_map", voiceregions, http));
	});
}


void cluster::guild_get_voice_regions(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "regions", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		voiceregion_map voiceregions;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_region : j) {
				voiceregions[string_not_null(&curr_region, "id")] = voiceregion().fill_from_json(&j);
			}
		}
		callback(confirmation_callback_t("voiceregion_map", voiceregions, http));
	});
}

};
