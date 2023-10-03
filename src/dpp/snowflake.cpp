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
#include <charconv>
#include <string>

namespace dpp {

snowflake::snowflake(std::string_view string_value) noexcept {
	auto [end, err] = std::from_chars(string_value.data(), string_value.data() + string_value.size(), value);
	if (end != string_value.data() + string_value.size())
		value = 0;
}

snowflake& snowflake::operator=(std::string_view string_value) noexcept {
	auto [end, err] = std::from_chars(string_value.data(), string_value.data() + string_value.size(), value);
	if (end != string_value.data() + string_value.size())
		value = 0;
	return *this;
}

snowflake::operator json() const {
	/* Discord transfers snowflakes as strings for compatibility with javascript */
	return std::to_string(value);
}

} // namespace dpp
