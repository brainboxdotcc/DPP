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
#include <dpp/snowflake.h>
#include <memory>

namespace dpp {

struct openssl_bignum;

/**
* @brief An arbitrary length integer number
*/
class DPP_EXPORT bignumber {
	std::shared_ptr<openssl_bignum> ssl_bn{nullptr};
public:
	/**
	 * @brief Construct a new bignumber object
	 */
	bignumber() = default;

	bignumber(const std::string& number_string);

	bignumber(std::vector<uint64_t> bits);

	~bignumber() = default;

	[[nodiscard]] std::string get_number(bool hex = false) const;

	[[nodiscard]] std::vector<uint64_t> get_binary() const;
};

} // namespace dpp
