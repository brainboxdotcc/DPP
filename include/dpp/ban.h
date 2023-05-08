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
#include <dpp/json_fwd.h>
#include <dpp/json_interface.h>
#include <unordered_map>

namespace dpp {

/**
 * @brief The ban class represents a ban on a guild.
 * 
 */
class DPP_EXPORT ban : public json_interface<ban> {
public:
	/** The ban reason */
	std::string reason;
	/** User ID the ban applies to */
	snowflake user_id;
	
	/** Constructor */
	ban();

	/** Destructor */
	virtual ~ban() = default;

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	 ban& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build json representation of a ban
	 * @param with_id Include ID in json
	 * 
	 * @return std::string stringified json
	 */
	std::string build_json(bool with_id = false) const;
};

/**
 * A group of bans. The key is the user ID
 */
typedef std::unordered_map<snowflake, ban> ban_map;

};
