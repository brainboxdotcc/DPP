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
 * This folder is a modified fork of libdave, https://github.com/discord/libdave
 * Copyright (c) 2024 Discord, Licensed under MIT
 *
 ************************************************************************************/
#include "util.h"

namespace dpp::dave::mls {

::mlspp::bytes_ns::bytes big_endian_bytes_from(uint64_t value) noexcept {
	auto buffer = ::mlspp::bytes_ns::bytes();
	buffer.reserve(sizeof(value));

	for (int i = sizeof(value) - 1; i >= 0; --i) {
		buffer.push_back(static_cast<uint8_t>(value >> (i * 8)));
	}

	return buffer;
}

uint64_t from_big_endian_bytes(const ::mlspp::bytes_ns::bytes& buffer) noexcept {
	uint64_t val = 0;

	if (buffer.size() <= sizeof(val)) {
		for (uint8_t byte : buffer) {
			val = (val << 8) | byte;
		}
	}

	return val;
}

}
