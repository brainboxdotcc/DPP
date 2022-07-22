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
#pragma once
#include <dpp/export.h>
#include <dpp/snowflake.h>
#include <dpp/misc-enum.h>
#include <dpp/managed.h>
#include <dpp/nlohmann/json_fwd.hpp>
#include <unordered_map>
#include <dpp/json_interface.h>

namespace dpp {

/**
 * @brief Defines types of webhook
 */
enum webhook_type {
	w_incoming = 1,		//!< Incoming webhook
	w_channel_follower = 2	//!< Channel following webhook
};

/**
 * @brief Represents a discord webhook
 */
class DPP_EXPORT webhook : public managed, public json_interface<webhook>  {
public:
	uint8_t type;   		//!< the type of the webhook
	snowflake guild_id;     	//!< Optional: the guild id this webhook is for
	snowflake channel_id;   	//!< the channel id this webhook is for
	snowflake user_id;		//!< Optional: the user this webhook was created by (not returned when getting a webhook with its token)
	std::string name;		//!< the default name of the webhook (may be empty)
	std::string avatar;		//!< the default avatar of the webhook (may be empty)
	std::string token;		//!< Optional: the secure token of the webhook (returned for Incoming Webhooks)
	snowflake application_id;	//!< the bot/OAuth2 application that created this webhook (may be empty)
	std::string* image_data;	//!< base64 encoded image data if uploading a new image

	/**
	 * @brief Construct a new webhook object
	 */
	webhook();

	/**
	 * @brief Construct a new webhook object using the Webhook URL provided by Discord
	 *
	 * @param webhook_url a fully qualified web address of an existing webhook
	 */
	webhook(const std::string& webhook_url);

	/**
	 * @brief Construct a new webhook object using the webhook ID and the webhook token
	 *
	 * @param webhook_id id taken from a link of an existing webhook
	 * @param webhook_token token taken from a link of an existing webhook
	 */
	webhook(const snowflake webhook_id, const std::string& webhook_token);

	/**
	 * @brief Destroy the webhook object
	 */
	~webhook();

	/**
	 * @brief Fill in object from json data
	 * 
	 * @param j JSON data
	 * @return webhook& Reference to self
	 */
	 webhook& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build JSON string from object
	 * 
	 * @param with_id Include the ID of the webhook in the json
	 * @return std::string JSON encoded object
	 */
	virtual std::string build_json(bool with_id = false) const;

	/**
	 * @brief Base64 encode image data and allocate it to image_data
	 * 
	 * @param image_blob Binary image data
	 * @param type Image type
	 * @param is_base64_encoded True if the image data is already base64 encoded
	 * @return webhook& Reference to self
	 * @throw dpp::exception Image data is larger than the maximum size of 256 kilobytes
	 */
	webhook& load_image(const std::string &image_blob, const image_type type, bool is_base64_encoded = false);
};

/**
 * @brief A group of webhooks
 */
typedef std::unordered_map<snowflake, webhook> webhook_map;

};
