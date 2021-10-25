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

void cluster::current_user_edit(const std::string &nickname, const std::string& image_blob, const image_type type, command_completion_event_t callback) {
	json j = json::parse("{\"nickname\": null}");
	if (!nickname.empty()) {
		j["nickname"] = nickname;
	}
	if (!image_blob.empty()) {
		static const std::map<image_type, std::string> mimetypes = {
			{ i_gif, "image/gif" },
			{ i_jpg, "image/jpeg" },
			{ i_png, "image/png" }
		};
		if (image_blob.size() > MAX_EMOJI_SIZE) {
			throw dpp::exception("User icon file exceeds discord limit of 256 kilobytes");
		}
		j["avatar"] = "data:" + mimetypes.find(type)->second + ";base64," + base64_encode((unsigned char const*)image_blob.data(), (unsigned int)image_blob.length());
	}
	this->post_rest(API_PATH "/users", "@me", "", m_patch, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("user", user().fill_from_json(&j), http));
		}
	});
}

};