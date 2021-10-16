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
#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

/**
 * @brief The etf_parser class can serialise and deserialise ETF (Erlang Term Format)
 * into and out of an nlohmann::json object, so that layers above the websocket don't
 * have to be any different for handling ETF.
 */
class DPP_EXPORT etf_parser {
public:
	/** Constructor */
	etf_parser();

	/** Destructor */
	~etf_parser();

	void parse(const std::string& data, nlohmann::json& j);

	std::string build(const nlohmann::json& j);
};

};
