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

#include <chrono>

namespace dpp::dave {

/**
 * @brief An interface for a wrapper around chrono clocks
 */
class clock_interface {
public:
	/**
	 * @brief chrono steady clock
	 */
	using base_clock = std::chrono::steady_clock;

	/**
	 * @brief time point on a steady clock
	 */
	using time_point = base_clock::time_point;

	/**
	 * @brief duration on a steady clock
	 */
	using clock_duration = base_clock::duration;

	/**
	 * @brief Default destructor
	 */
	virtual ~clock_interface() = default;

	/**
	 * @brief Get current time
	 * @return current time
	 */
	virtual time_point now() const = 0;
};

/**
 * @brief Chrono clock class
 */
class clock : public clock_interface {
public:
	/**
	 * @brief Get current time
	 * @return current time
	 */
	time_point now() const override {
		return base_clock::now();
	}
};

}
