/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
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
	uint64_t value = 0;

public:
	/**
 	 * @brief Construct a snowflake object
 	 */
	constexpr snowflake() noexcept = default;

	/**
	 * @brief Construct a snowflake object
	 * @param value A snowflake value
	 */
	constexpr snowflake(const uint64_t value_) noexcept : value{value_} {}

	/**
	 * @brief Construct a snowflake object
	 * @param string_value A snowflake value
	 */
	snowflake(std::string_view string_value) noexcept;

	/**
	 * @brief For acting like an integer
	 * @return The snowflake value
	 */
	constexpr operator uint64_t() const noexcept {
		return value;
	}

	/**
	 * @brief Returns true if the snowflake holds an empty value (is 0)
	 *
	 * @return true if empty (zero)
	 */
	constexpr bool empty() const noexcept {
		return value == 0;
	}

	/**
	 * @brief Returns the stringified version of the snowflake value
	 *
	 * @return std::string string form of snowflake value
	 */
	inline std::string str() const {
		return std::to_string(value);
	}

	/**
	 * @brief Assign from std::string
	 *
	 * @param snowflake_val string to assign from.
	 */
	snowflake& operator=(std::string_view snowflake_val) noexcept;

	/**
	 * @brief Comparison operator with a string
	 *
	 * @param snowflake_val snowflake value as a string
	 */
	inline bool operator==(std::string_view snowflake_val) const noexcept {
		return *this == dpp::snowflake{snowflake_val};
	}

	/**
	 * @brief For acting like an integer
	 * @return A reference to the snowflake value
	 */
	constexpr operator uint64_t &() noexcept {
		return value;
	}

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
	constexpr double get_creation_time() const noexcept {
		constexpr uint64_t first_january_2016 = 1420070400000ull;
		return static_cast<double>((value >> 22) + first_january_2016) / 1000.0;
	}

	/**
	 * @brief Get the worker id that produced this snowflake value
	 *
	 * @return uint8_t worker id
	 */
	constexpr uint8_t get_worker_id() const noexcept {
		return static_cast<uint8_t>((value & 0x3E0000) >> 17);
	}

	/**
	 * @brief Get the process id that produced this snowflake value
	 *
	 * @return uint8_t process id
	 */
	constexpr uint8_t get_process_id() const noexcept {
		return static_cast<uint8_t>((value & 0x1F000) >> 12);
	}

	/**
	 * @brief Get the increment, which is incremented for every snowflake
	 * created over the one millisecond resolution in the timestamp.
	 *
	 * @return uint64_t millisecond increment
	 */
	constexpr uint16_t get_increment() const noexcept {
		return static_cast<uint16_t>(value & 0xFFF);
	}
};

} // namespace dpp

template<>
struct std::hash<dpp::snowflake>
{
	/**
	 * @brief Hashing function for dpp::snowflake
	 * Used by std::unordered_map. This just calls std::hash<uint64_t>.
	 *
	 * @param s Snowflake value to hash
	 * @return std::size_t hash value
	 */
	std::size_t operator()(dpp::snowflake s) const noexcept {
		return std::hash<uint64_t>{}(s.value);
	}
};
