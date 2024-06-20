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

#include <dpp/bignum.h>
#include <dpp/stringops.h>
#include <openssl/bn.h>
#include <cmath>

namespace dpp {

struct openssl_bignum {
	/**
	 * @brief OpenSSL BIGNUM pointer
	 */
	BIGNUM* bn{nullptr};

	/**
	 * @brief Construct BIGNUM using RAII
	 */
	openssl_bignum() : bn(BN_new()) {
	}

	/**
	 * @brief Destruct BIGNUM using RAII
	 */
	~openssl_bignum() {
		BN_free(bn);
	}
};

bignumber::bignumber(const std::string& number_string) : ssl_bn(std::make_shared<openssl_bignum>()) {
	if (dpp::lowercase(number_string.substr(0, 2)) == "0x") {
		BN_hex2bn(&ssl_bn->bn, number_string.substr(2, number_string.length() - 2).c_str());
	} else {
		BN_dec2bn(&ssl_bn->bn, number_string.c_str());
	}
}

/**
 * Flip (reverse) bytes in a uint64_t
 * @param bytes 64 bit value
 * @return flipped 64 bit value
 */
inline uint64_t flip_bytes(uint64_t bytes) {
	return ((((bytes) & 0xff00000000000000ull) >> 56)
	       | (((bytes) & 0x00ff000000000000ull) >> 40)
	       | (((bytes) & 0x0000ff0000000000ull) >> 24)
	       | (((bytes) & 0x000000ff00000000ull) >> 8)
	       | (((bytes) & 0x00000000ff000000ull) << 8)
	       | (((bytes) & 0x0000000000ff0000ull) << 24)
	       | (((bytes) & 0x000000000000ff00ull) << 40)
	       | (((bytes) & 0x00000000000000ffull) << 56));
}

bignumber::bignumber(std::vector<uint64_t> bits): ssl_bn(std::make_shared<openssl_bignum>()) {
	std::reverse(bits.begin(), bits.end());
	for (auto& chunk : bits) {
		chunk = flip_bytes(chunk);
	}
	BN_bin2bn(reinterpret_cast<unsigned char*>(bits.data()), bits.size() * sizeof(uint64_t), ssl_bn->bn);
}

std::string bignumber::get_number(bool hex) const {
	char* number_str = hex ? BN_bn2hex(ssl_bn->bn) : BN_bn2dec(ssl_bn->bn);
	std::string returned{number_str};
	OPENSSL_free(number_str);
	return returned;
}

std::vector<uint64_t> bignumber::get_binary() const {
	std::size_t size = BN_num_bytes(ssl_bn->bn);
	auto size_64_bit = static_cast<std::size_t>(ceil(static_cast<double>(size) / sizeof(uint64_t)));
	std::vector<uint64_t> returned;
	returned.resize(size_64_bit);
	BN_bn2binpad(ssl_bn->bn, reinterpret_cast<unsigned char*>(returned.data()), returned.size() * sizeof(uint64_t));
	std::reverse(returned.begin(), returned.end());
	for (auto& chunk : returned) {
		chunk = flip_bytes(chunk);
	}
	return returned;
}

}
