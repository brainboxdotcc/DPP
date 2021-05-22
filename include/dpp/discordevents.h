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

#include <dpp/json_fwd.hpp>

namespace dpp {

/** @brief Returns a snowflake id from a json field value, if defined, else returns 0
 * @param j nlohmann::json instance to retrieve value from
 * @param keyname key name to check for a value
 */
uint64_t SnowflakeNotNull(const nlohmann::json* j, const char *keyname);

/** @brief Returns a string from a json field value, if defined, else returns an empty string.
 * @param j nlohmann::json instance to retrieve value from
 * @param keyname key name to check for a value
 */
std::string StringNotNull(const nlohmann::json* j, const char *keyname);

/** @brief Returns a 64 bit unsigned integer from a json field value, if defined, else returns 0.
 * DO NOT use this for snowflakes, as usually snowflakes are wrapped in a string!
 * @param j nlohmann::json instance to retrieve value from
 * @param keyname key name to check for a value
 */
uint64_t Int64NotNull(const nlohmann::json* j, const char *keyname);

/** @brief Returns a 32 bit unsigned integer from a json field value, if defined, else returns 0
 * @param j nlohmann::json instance to retrieve value from
 * @param keyname key name to check for a value
 */
uint32_t Int32NotNull(const nlohmann::json* j, const char *keyname);

/** @brief Returns a 16 bit unsigned integer from a json field value, if defined, else returns 0
 * @param j nlohmann::json instance to retrieve value from
 * @param keyname key name to check for a value
 */
uint16_t Int16NotNull(const nlohmann::json* j, const char *keyname);

/** @brief Returns an 8 bit unsigned integer from a json field value, if defined, else returns 0
 * @param j nlohmann::json instance to retrieve value from
 * @param keyname key name to check for a value
 */
uint8_t Int8NotNull(const nlohmann::json* j, const char *keyname);

/** @brief Returns a boolean value from a json field value, if defined, else returns false
 * @param j nlohmann::json instance to retrieve value from
 * @param keyname key name to check for a value
 */
bool BoolNotNull(const nlohmann::json* j, const char *keyname);

/** @brief Returns a time_t from an ISO8601 timestamp field in a json value, if defined, else returns
 * epoch value of 0.
 * @param j nlohmann::json instance to retrieve value from
 * @param keyname key name to check for a value
 */
time_t TimestampNotNull(const nlohmann::json* j, const char *keyname);


/** @brief Base64 encode data.
 * @param buf Raw buffer
 * @param buffer_length Buffer length to encode
 */
std::string base64_encode(unsigned char const* buf, unsigned int buffer_length);

};
