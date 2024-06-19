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

/**
 * @brief This contains the OpenSSL structs. It is not public,
 * so that the public interface doesn't depend on OpenSSL directly.
 */
struct openssl_bignum;

/**
* @brief An arbitrary length integer number.
 * Officially, the Discord documentation says that permission values can be any arbitrary
 * number of digits. At time of writing there are only 50 bits of permissions, but this is
 * set to grow larger and potentially past 64 bits. They will continue to send this data
 * as a huge single integer at that point, because this is obviously sensible. /s
 *
 * @note dpp::bignumber uses OpenSSL BN_* under the hood, as we include openssl anyway
 * for HTTPS.
*/
class DPP_EXPORT bignumber {
	/**
	 * @brief Internal opaque struct to contain OpenSSL things
	 */
	std::shared_ptr<openssl_bignum> ssl_bn{nullptr};
public:
	/**
	 * @brief Construct a new bignumber object
	 */
	bignumber() = default;

	/**
	 * @brief Parse a std::string of an arbitrary length number into
	 * a bignumber.
	 * @param number_string string representation of a number. The
	 * number must be an integer, and can be positive or negative.
	 * @note Prefixing number_string with 0x will parse it as hexadecimal.
	 * This is not case sensitive.
	 */
	bignumber(const std::string& number_string);

	/**
	 * @brief Build a bignumber from a vector of 64 bit values.
	 * The values are accepted in "reverse order", so the first vector
	 * entry at index 0 is the leftmost 64 bits of the bignum.
	 * The vector can be any arbitrary length.
	 * @param bits Vector of 64 bit values which represent the number
	 */
	bignumber(std::vector<uint64_t> bits);

	/**
	 * @brief Default destructor
	 */
	~bignumber() = default;

	/**
	 * @brief Get the string representation of the bignumber.
	 * @param hex If false (the default) the number is returned in
	 * decimal, else if this parameter is true, it will be returned
	 * as hex (without leading '0x')
	 * @return String representation of bignumber
	 */
	[[nodiscard]] std::string get_number(bool hex = false) const;

	/**
	 * @brief Get the array of 64 bit values that represents the
	 * bignumber. This is what we should use to store bignumbers
	 * in memory, not this bignumber class itself, as the bignumber
	 * class instantiates OpenSSL structs and takes significantly
	 * more ram than just a vector.
	 * @return Vector of 64 bit values representing the bignumber
	 */
	[[nodiscard]] std::vector<uint64_t> get_binary() const;
};

} // namespace dpp
