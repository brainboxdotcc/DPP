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

#include <algorithm>
#include <functional>
#include <utility>

namespace dpp::dave {

/**
 * @brief Calls a lambda when the class goes out of scope
 */
class [[nodiscard]] scope_exit final {
public:
	/**
	 * Create new scope_exit with a lambda
	 * @tparam Cleanup lambda type
	 * @param cleanup lambda
	 */
	template <typename Cleanup> explicit scope_exit(Cleanup&& cleanup) : exit_function{std::forward<Cleanup>(cleanup)} {
	}

	/**
	 * @brief Move constructor
	 * @param rhs other object
	 */
	scope_exit(scope_exit&& rhs) : exit_function{std::move(rhs.exit_function)} {
		rhs.exit_function = nullptr;
	}

	/**
	 * @brief Calls lambda
	 */
	~scope_exit() {
		if (exit_function) {
			exit_function();
		}
	}

	/**
	 * @brief move assignment
	 * @param rhs other object
	 * @return self
	 */
	scope_exit& operator=(scope_exit&& rhs) {
		exit_function = std::move(rhs.exit_function);
		rhs.exit_function = nullptr;
		return *this;
	}

	/**
	 * @brief Clear the lambda so it isn't called
	 */
	void dismiss() {
		exit_function = nullptr;
	}

private:
	/**
	 * @brief Deleted copy constructor
	 */
	scope_exit(scope_exit const&) = delete;

	/**
	 * @brief Deleted assignment operator
	 * @return
	 */
	scope_exit& operator=(scope_exit const&) = delete;

	/**
	 * @brief Lambda to call
	 */
	std::function<void()> exit_function;
};

}
