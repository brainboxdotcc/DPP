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
#pragma once

#include <memory>
#include "common.h"

namespace dpp::dave {

/**
 * @brief Key generation number
 */
using key_generation = uint32_t;

/**
 * @brief Represents the interface used for a key ratchet.
 * A key ratchet is a way of propogating key pairs up the tree used by MLS
 * so we dont have to store O(n^n) key combinations.
 */
class key_ratchet_interface { // NOLINT
public:
	/**
	 * @brief Default destructor
	 */
	virtual ~key_ratchet_interface() noexcept = default;

	/**
	 * @brief Get key for ratchet
	 * @param generation current generation number
	 * @return encryption key
	 */
	virtual encryption_key get_key(key_generation generation) noexcept = 0;

	/**
	 * @brief Delete key for ratchet
	 * @param generation current generation number
	 */
	virtual void delete_key(key_generation generation) noexcept = 0;
};

}
