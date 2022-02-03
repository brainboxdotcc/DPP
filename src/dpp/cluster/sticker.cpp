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
#include <dpp/message.h>
#include <dpp/cluster.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

void cluster::guild_sticker_create(sticker &s, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(s.guild_id), "stickers", m_post, s.build_json(false), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("sticker", sticker().fill_from_json(&j), http));
		}
	}, s.filename, s.filecontent);
}


void cluster::guild_sticker_delete(snowflake sticker_id, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "stickers/" + std::to_string(sticker_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::guild_sticker_get(snowflake id, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "stickers/" + std::to_string(id), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("sticker", sticker().fill_from_json(&j), http));
		}
	});
}


void cluster::guild_sticker_modify(sticker &s, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(s.guild_id), "stickers/" + std::to_string(s.id), m_patch, s.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("sticker", sticker().fill_from_json(&j), http));
		}
	});
}


void cluster::guild_stickers_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "stickers", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		sticker_map stickers;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_sticker : j) {
				stickers[snowflake_not_null(&curr_sticker, "id")] = sticker().fill_from_json(&curr_sticker);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("sticker_map", stickers, http));
		}
	});
}


void cluster::nitro_sticker_get(snowflake id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/stickers", std::to_string(id), "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("sticker", sticker().fill_from_json(&j), http));
		}
	});
}


void cluster::sticker_packs_get(command_completion_event_t callback) {
	this->post_rest(API_PATH "/sticker-packs", "", "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		sticker_pack_map stickerpacks;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_stickerpack : j) {
				stickerpacks[snowflake_not_null(&curr_stickerpack, "id")] = sticker_pack().fill_from_json(&curr_stickerpack);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("sticker_pack_map", stickerpacks, http));
		}
	});
}

};