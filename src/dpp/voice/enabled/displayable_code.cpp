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

#include <sstream>
#include <iomanip>
#include <cmath>
#include <dpp/exception.h>

#include "../../dave/encryptor.h"

namespace dpp {

std::string generate_displayable_code(const std::vector<uint8_t> &data, size_t desired_length = 30, size_t group_size = 5) {

	if (data.empty()) {
		return "";
	}

	const size_t group_modulus = std::pow(10, group_size);
	std::stringstream result;

	for (size_t i = 0; i < desired_length; i += group_size) {
		size_t group_value{0};

		for (size_t j = group_size; j > 0; --j) {
			const size_t next_byte = data.at(i + (group_size - j));
			group_value = (group_value << 8) | next_byte;
		}
		group_value %= group_modulus;
		result << std::setw(group_size) << std::setfill('0') << std::to_string(group_value) << " ";
	}

	return result.str();
}

}
