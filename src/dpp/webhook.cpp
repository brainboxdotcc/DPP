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
#include <dpp/webhook.h>
#include <dpp/discordevents.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/exception.h>

namespace dpp {

using json = nlohmann::json;

const size_t MAX_ICON_SIZE = 256 * 1024;

webhook::webhook() : managed(), type(w_incoming), guild_id(0), channel_id(0), user_id(0), application_id(0), image_data(nullptr)
{
}

webhook::webhook(const std::string& webhook_url) : webhook()
{
	token = webhook_url.substr(webhook_url.find_last_of("/") + 1);
	try {
		id = std::stoull(webhook_url.substr(strlen("https://discord.com/api/webhooks/"), webhook_url.find_last_of("/")));
	}
	catch (const std::exception& e) {
		throw dpp::logic_exception(std::string("Failed to parse webhook URL: ") + e.what());
	}
}

webhook::webhook(const snowflake webhook_id, const std::string& webhook_token) : webhook()
{
	token = webhook_token;
	id = webhook_id;
}

webhook::~webhook() {
	if (image_data) {
		delete image_data;
	}
}

webhook& webhook::fill_from_json(nlohmann::json* j) {
	id = snowflake_not_null(j, "id");
	type = int8_not_null(j, "type");
	channel_id = snowflake_not_null(j, "channel_id");
	guild_id = snowflake_not_null(j, "guild_id");
	if (j->find("user") != j->end()) {
		json & user = (*j)["user"];
		user_id = snowflake_not_null(&user, "id");
	}
	name = string_not_null(j, "name");
	avatar = string_not_null(j, "name");
	token = string_not_null(j, "token");
	application_id = snowflake_not_null(j, "application_id");

	return *this;
}

std::string webhook::build_json(bool with_id) const {
	json j;
	if (with_id) {
		j["id"] = std::to_string(id);
	}
	j["name"] = name;
	j["type"] = type;
	if (channel_id)
		j["channel_id"] = channel_id;
	if (guild_id)
		j["guild_id"] = guild_id;
	if (!name.empty())
		j["name"] = name;
	if (image_data)
		j["avatar"] = *image_data;
	if (application_id)
		j["application_id"] = application_id;
	return j.dump();
}

webhook& webhook::load_image(const std::string &image_blob, const image_type type, bool is_base64_encoded) {
	static const std::map<image_type, std::string> mimetypes = {
		{ i_gif, "image/gif" },
		{ i_jpg, "image/jpeg" },
		{ i_png, "image/png" }
	};
	if (image_blob.size() > MAX_ICON_SIZE) {
		throw dpp::length_exception("Webhook icon file exceeds discord limit of 256 kilobytes");
	}

	/* If there's already image data defined, free the old data, to prevent a memory leak */
	delete image_data;
	image_data = new std::string("data:" + mimetypes.find(type)->second + ";base64," + (is_base64_encoded ? image_blob : base64_encode((unsigned char const*)image_blob.data(), (unsigned int)image_blob.length())));

	return *this;
}

};

