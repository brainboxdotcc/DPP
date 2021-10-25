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
#include <map>
#include <dpp/discord.h>
#include <dpp/cluster.h>
#include <dpp/discordclient.h>
#include <dpp/discordevents.h>
#include <dpp/message.h>
#include <dpp/cache.h>
#include <dpp/nlohmann/json.hpp>
#include <utility>
#include <dpp/fmt/format.h>

namespace dpp {

void cluster::edit_webhook_with_token(const class webhook& wh, command_completion_event_t callback) {
	json jwh;
	try {
		jwh = json::parse(wh.build_json(true));
	}
	catch (const std::exception &e) {
		log(ll_error, fmt::format("edit_webhook_with_token(): {}", e.what()));
		return;
	}
	if (jwh.find("channel_id") != jwh.end()) {
		jwh.erase(jwh.find("channel_id"));
	}
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), dpp::url_encode(wh.token), m_patch, jwh.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("webhook", webhook().fill_from_json(&j), http));
		}
	});
}

};