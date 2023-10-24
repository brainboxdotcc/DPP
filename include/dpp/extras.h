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
#include <dpp/utility.h>
#include <dpp/restresults.h>
#include <functional>
#include <variant>
#include <tuple>

namespace dpp {

namespace utility {

/**
 * @brief Structure to access traits of a callable. For example how many arguments it has and its return type
 */
template <typename T>
struct function_traits;

template <typename R, typename... Args>
struct function_traits<std::function<R(Args...)>> {
	/**
	 * @brief Number of arguments to the function.
	 */
	inline static constexpr size_t n_args = sizeof...(Args);

	/** @brief Arguments as a tuple to get the list of arguments and use tuple type traits. */
	using args_tuple = std::tuple<Args...>;

	/** @brief Return type of the function. */
	using return_type = R;

	/**
	 * @brief Type alias to the Nth argument to the function.
	 * @tparam Idx Index of the argument in the argument list
	 */
	template <size_t Idx>
	using arg = std::tuple_element_t<Idx, args_tuple>;
};

/**
 * @brief Type trait constexpr variable to check if a type T is one of the possible types in a variant V.
 *
 * @tparam T Type to find in variant
 * @tparam V Variant to check
 */
template <typename T, typename V>
inline constexpr bool variant_has_v = false;

template <typename T, typename... Args>
inline constexpr bool variant_has_v<T, std::variant<Args...>> = (std::is_same_v<T, Args> || ...);

/**
 * @brief Structure to access traits of a callable. For example how many arguments it has and its return type
 *
 * This is a type alias to \ref function_traits for convenience
 * @tparam Type of the callable
 * @see function_traits
 */
template <typename T>
using function_traits_t = function_traits<decltype(std::function{std::declval<T&&>()})>;

/**
 * @brief Type trait to get the Idx-th argument to a function-like type.
 * @tparam Type of the callable
 * @tparam Idx Index of the argument in the callable's argument list
 */
template <typename T, size_t Idx = 0>
using function_arg_t = typename function_traits_t<T>::template arg<Idx>;

/**
 * @brief Convenience function to generate a function suitable for use as a callback to API calls. On success calls the given function, on error log the error to the cluster.
 *
 * Example:
 * \code{.cpp}
 * bot.message_create(message, dpp::utility::if_success([&bot](const dpp::message &m) {
 *     bot.log(dpp::ll_info, "message sent successfully");
 * }));
 * \encode
 *
 * @param on_success Function to use on success.
 */
template <typename Callable>
dpp::command_completion_event_t if_success(Callable&& on_success) {
	if constexpr (variant_has_v<remove_cvref_t<function_arg_t<Callable, 0>>, dpp::confirmable_t>) {
		using fn_arg = std::remove_cv_t<std::remove_reference_t<function_arg_t<Callable, 0>>>;

		return [cb = std::forward<Callable>(on_success)](const dpp::confirmation_callback_t &callback) {
			if (callback.is_error()) {
				if (callback.bot) {
					callback.bot->log(
						dpp::ll_error,
						"Error: " + callback.get_error().human_readable
					);
				}
			} else {
				if (std::holds_alternative<fn_arg>(callback.value)) {
					std::invoke(cb, std::get<fn_arg>(callback.value));
				} else {
					throw dpp::logic_exception("wrong argument type for callback data in utility::if_success");
				}
			}
		};
	} else {
		static_assert(!std::is_same_v<std::void_t<Callable>, void>, "invalid parameter to if_success, type is not a callback type");

		return {};
	}
}

}

}
