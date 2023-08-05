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
#include <dpp/snowflake.h>
#include <dpp/json.h>

namespace dpp {

snowflake::snowflake(const uint64_t &value) : value(value) {}

snowflake::snowflake(const std::string &string_value) {
	try {
		value = std::stoull(string_value);
	}
	catch (const std::exception &) {
		value = 0;
	}
}

snowflake::snowflake() : snowflake(0) {}

snowflake::operator uint64_t() const {
	return value;
}

snowflake::operator uint64_t &() {
	return value;
}

snowflake& snowflake::operator=(const std::string &snowflake_val) {
	try {
		value = std::stoull(snowflake_val);
	}
	catch (const std::exception &) {
		value = 0;
	}
	return *this;
}

snowflake& snowflake::operator=(const uint64_t &snowflake_val) {
	value = snowflake_val;
	return *this;
}

bool snowflake::operator==(const snowflake& other) const {
	return other.value == value;
}

bool snowflake::operator==(const uint64_t& other) const {
	return other == value;
}

snowflake::operator nlohmann::json() const {
	/* Discord transfers snowflakes as strings for compatibility with javascript */
	return std::to_string(value);
}

double snowflake::get_creation_time() const {
	const uint64_t first_january_2016 = 1420070400000;
	return (double)((value >> 22) + first_january_2016) / 1000.0;
}

uint8_t snowflake::get_worker_id() const {
	return (uint8_t)((value & 0x3E0000) >> 17);
}

uint8_t snowflake::get_process_id() const {
	return (uint8_t)((value & 0x1F000) >> 12);
}

uint16_t snowflake::get_increment() const {
	return (value & 0xFFF);
}

} // namespace dpp
