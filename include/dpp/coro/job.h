/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2022 Craig Edwards and D++ contributors
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

#ifdef DPP_CORO
#pragma once

#include "coro.h"

#include <type_traits>
#include <utility>

namespace dpp {

/**
 * @brief Extremely light coroutine object designed to send off a coroutine to execute on its own. It can be attached to an event router using dpp::event_router_t::co_attach.
 *
 * This object stores no state and is the recommended way to use coroutines if you do not need to co_await the result.
 *
 * @warning - This feature is EXPERIMENTAL. The API may change at any time and there may be bugs. Please report any to <a href="https://github.com/brainboxdotcc/DPP/issues">GitHub issues</a> or to the <a href="https://discord.gg/dpp">D++ Discord server</a>.
 * @warning - It cannot be co_awaited, which means the second it co_awaits something, the program jumps back to the calling function, which continues executing.
 * At this point, if the function returns, every object declared in the function including its parameters are destroyed, which causes dangling references.
 * This is exactly the same problem as references in lambdas : https://dpp.dev/lambdas-and-locals.html.
 * For this reason, `co_await` will error if any parameters are passed by reference.
 * If you must pass a reference, pass it as a pointer or with std::ref, but you must fully understand the reason behind this warning, and what to avoid.
 * If you prefer a safer type, use `coroutine` for synchronous execution, or `task` for parallel tasks, and co_await them.
 */
struct job {};

namespace detail {

template <typename... Args>
inline constexpr bool coroutine_has_ref_params_v = (std::is_reference_v<Args> || ... || false);

template <typename T, typename... Args>
inline constexpr bool coroutine_has_ref_params_v<T, Args...> = (std::is_reference_v<Args> || ... || (std::is_reference_v<T> && !std::is_invocable_v<T, Args...>));

#ifdef DPP_CORO_TEST
	struct job_promise_base{};
#endif

/**
 * @brief Coroutine promise type for a job
 */
template <typename... Args>
struct job_promise {

#ifdef DPP_CORO_TEST
	job_promise() {
		++coro_alloc_count<job_promise_base>;
	}

	~job_promise() {
		--coro_alloc_count<job_promise_base>;
	}
#endif

	/*
	* @brief Function called when the job is done.
	*
	* @return Do not suspend at the end, destroying the handle immediately
	*/
	std_coroutine::suspend_never final_suspend() const noexcept {
		return {};
	}

	/*
	* @brief Function called when the job is started.
	*
	* @return Do not suspend at the start, starting the job immediately
	*/
	std_coroutine::suspend_never initial_suspend() const noexcept {
		return {};
	}

	/**
	 * @brief Function called to get the job object
	 *
	 * @return job
	 */
	dpp::job get_return_object() const noexcept {
		return {};
	}

	/**
	 * @brief Function called when an exception is thrown and not caught.
	 *
	 * @throw Immediately rethrows the exception to the caller / resumer
	 */
	void unhandled_exception() const {
		throw;
	}

	/**
	 * @brief Function called when the job returns. Does nothing.
	 */
	void return_void() const noexcept {}

	template <typename T>
	T await_transform(T &&expr) const noexcept {
		/**
		 * `job` is extremely efficient as a coroutine but this comes with drawbacks :
		 * It cannot be co_awaited, which means the second it co_awaits something, the program jumps back to the calling function, which continues executing.
		 * At this point, if the function returns, every object declared in the function including its parameters are destroyed, which causes dangling references.
		 * This is exactly the same problem as references in lambdas : https://dpp.dev/lambdas-and-locals.html.
		 *
		 * If you must pass a reference, pass it as a pointer or with std::ref, but you must fully understand the reason behind this warning, and what to avoid.
		 * If you prefer a safer type, use `coroutine` for synchronous execution, or `task` for parallel tasks, and co_await them.
		 */
		static_assert(!coroutine_has_ref_params_v<Args...>, "co_await is disabled in dpp::job when taking parameters by reference. read comment above this line for more info");

		return std::forward<T>(expr);
	}
};

} // namespace detail

} // namespace dpp

/**
 * @brief Specialization of std::coroutine_traits, helps the standard library figure out a promise type from a coroutine function.
 */
template<typename... Args>
struct dpp::detail::std_coroutine::coroutine_traits<dpp::job, Args...> {
	/**
	 * @brief Promise type for this coroutine signature.
	 *
	 * When the coroutine is created from a lambda, that lambda is passed as a first parameter.
	 * Not ideal but we'll allow any callable that takes the rest of the arguments passed
	 */
	using promise_type = dpp::detail::job_promise<Args...>;
};

#endif /* DPP_CORO */
