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
#include <dpp/json_fwd.h>
#include <stdint.h>
#include <functional>

/**
 * @brief The main namespace for D++ functions. classes and types
 */
namespace dpp {

/** @brief A container for a 64 bit unsigned value representing many things on discord.
 * This value is known in distributed computing as a snowflake value.
 * 
 * Snowflakes are:
 * 
 * - Performant (very fast to generate at source and to compare in code)
 * - Uncoordinated (allowing high availability across clusters, data centres etc)
 * - Time ordered (newer snowflakes have higher IDs)
 * - Directly Sortable (due to time ordering)
 * - Compact (64 bit numbers, not 128 bit, or string)
 * 
 * An identical format of snowflake is used by Twitter, Instagram and several other platforms.
 * 
 * @see https://en.wikipedia.org/wiki/Snowflake_ID
 * @see https://github.com/twitter-archive/snowflake/tree/b3f6a3c6ca8e1b6847baa6ff42bf72201e2c2231
 */
class DPP_EXPORT snowflake final {
	friend struct std::hash<dpp::snowflake>;
protected:
	/**
	 * @brief The snowflake value
	 */
	uint64_t value;

public:
	/**
	 * @brief Construct a snowflake object
	 * @param value A snowflake value
	 */
	snowflake(const uint64_t& value);

	/**
	 * @brief Construct a snowflake object
	 * @param string_value A snowflake value
	 */
	snowflake(const std::string& string_value);

	/**
 	 * @brief Construct a snowflake object
 	 */
	snowflake();

	/**
	 * @brief Destroy the snowflake object
	 */
	~snowflake() = default;

	/**
	 * @brief For acting like an integer
	 * @return The snowflake value
	 */
	operator uint64_t() const;

	/**
	 * @brief Returns true if the snowflake holds an empty value (is 0)
	 * 
	 * @return true if empty (zero)
	 */
	inline bool empty() const
	{
		return value == 0;
	}

	/**
	 * @brief Operator less than, used for maps/unordered maps
	 * when the snowflake is a key value.
	 * 
	 * @param lhs fist snowflake to compare
	 * @param rhs second snowflake to compare
	 * @return true if lhs is less than rhs
	 */
	friend inline bool operator< (const snowflake& lhs, const snowflake& rhs)
	{
		return lhs.value < rhs.value;
	}

	/**
	 * @brief Assign from std::string
	 * 
	 * @param snowflake_val string to assign from.
	 */
	snowflake& operator=(const std::string &snowflake_val);

	/**
	 * @brief Assign from std::string
	 * 
	 * @param snowflake_val value to assign from.
	 */
	snowflake& operator=(const uint64_t &snowflake_val);

	/**
	 * @brief Check if one snowflake value is equal to another
	 * 
	 * @param other other snowflake to compare
	 * @return True if the snowflake objects match
	 */
	bool operator==(const snowflake& other) const;

	/**
	 * @brief Check if one snowflake value is equal to a uint64_t
	 * 
	 * @param other other snowflake to compare
	 * @return True if the snowflake objects match
	 */
	bool operator==(const uint64_t& other) const;

	/**
	 * @brief For acting like an integer
	 * @return A reference to the snowflake value
	 */
	operator uint64_t &();

	/**
	 * @brief For building json
	 * @return The snowflake value as a string
	 */
	operator nlohmann::json() const;

	/**
	 * @brief Get the creation time of this snowflake according to Discord.
	 * 
	 * @return double creation time inferred from the snowflake ID.
	 * The minimum possible value is the first second of 2015.
	 */
	double get_creation_time() const;

	/**
	 * @brief Get the worker id that produced this snowflake value
	 * 
	 * @return uint8_t worker id
	 */
	uint8_t get_worker_id() const;

	/**
	 * @brief Get the process id that produced this snowflake value
	 * 
	 * @return uint8_t process id
	 */
	uint8_t get_process_id() const;

	/**
	 * @brief Get the increment, which is incremented for every snowflake
	 * created over the one millisecond resolution in the timestamp.
	 * 
	 * @return uint64_t millisecond increment
	 */
	uint16_t get_increment() const;
};

};

template<>
struct std::hash<dpp::snowflake>
{
	/**
	 * @brief Hashing function for dpp::slowflake
	 * Used by std::unordered_map. This just calls std::hash<uint64_t>.
	 * 
	 * @param s Snowflake value to hash
	 * @return std::size_t hash value
	 */
	std::size_t operator()(dpp::snowflake const& s) const noexcept {
		return std::hash<uint64_t>{}(s.value);
	}
};
