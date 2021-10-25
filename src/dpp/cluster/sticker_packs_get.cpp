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

namespace dpp {

void cluster::sticker_packs_get(command_completion_event_t callback) {
	this->post_rest(API_PATH "/sticker-packs", "", "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		sticker_pack_map stickerpacks;
		for (auto & curr_stickerpack : j) {
			stickerpacks[SnowflakeNotNull(&curr_stickerpack, "id")] = sticker_pack().fill_from_json(&curr_stickerpack);
		}
		if (callback) {
			callback(confirmation_callback_t("sticker_pack_map", stickerpacks, http));
		}
	});
}

};