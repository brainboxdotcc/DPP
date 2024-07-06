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
#pragma once

#include <dpp/utility.h>

#include <dpp/coro/awaitable.h>

namespace dpp {

struct async_dummy : awaitable_dummy {
	std::shared_ptr<int> dummy_shared_state = nullptr;
};

}

#ifdef DPP_CORO

#include "coro.h"

#include <utility>
#include <type_traits>
#include <functional>
#include <atomic>
#include <cstddef>

namespace dpp {

namespace detail {

namespace async {

/**
 * @brief Shared state of the async and its callback, to be used across threads.
 */
template <typename R>
struct callback {
	std::shared_ptr<basic_promise<R>> promise{nullptr};

	void operator()(const R& v) const {
		promise->set_value(v);
	}

	void operator()(R&& v) const {
		promise->set_value(std::move(v));
	}
};

template <>
struct callback<void> {
	std::shared_ptr<basic_promise<void>> promise{nullptr};

	void operator()() const {
		promise->set_value();
	}
};

} // namespace async

} // namespace detail

struct confirmation_callback_t;

/**
 * @class async async.h coro/async.h
 * @brief A co_await-able object handling an API call in parallel with the caller.
 *
 * This class is the return type of the dpp::cluster::co_* methods, but it can also be created manually to wrap any async call.
 *
 * @remark - The coroutine may be resumed in another thread, do not rely on thread_local variables.
 * @warning - This feature is EXPERIMENTAL. The API may change at any time and there may be bugs. Please report any to <a href="https://github.com/brainboxdotcc/DPP/issues">GitHub issues</a> or to the <a href="https://discord.gg/dpp">D++ Discord server</a>.
 * @tparam R The return type of the API call. Defaults to confirmation_callback_t
 */
template <typename R>
class async : public awaitable<R> {

	detail::async::callback<R> api_callback{};

	explicit async(std::shared_ptr<basic_promise<R>> &&promise) : awaitable<R>{promise.get()}, api_callback{std::move(promise)} {}

public:
	using awaitable<R>::awaitable; // use awaitable's constructors
	using awaitable<R>::operator=; // use async_base's assignment operator
	using awaitable<R>::await_ready; // expose await_ready as public

	/**
	 * @brief The return type of the API call. Defaults to confirmation_callback_t
	 */
	using result_type = R;

	/**
	 * @brief Construct an async object wrapping an object method, the call is made immediately by forwarding to <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a> and can be awaited later to retrieve the result.
	 *
	 * @param obj The object to call the method on
	 * @param fun The method of the object to call. Its last parameter must be a callback taking a parameter of type R
	 * @param args Parameters to pass to the method, excluding the callback
	 */
	template <typename Obj, typename Fun, typename... Args>
#ifndef _DOXYGEN_
	requires std::invocable<Fun, Obj, Args..., std::function<void(R)>>
#endif
	explicit async(Obj &&obj, Fun &&fun, Args&&... args) : async{std::make_shared<basic_promise<R>>()} {
		std::invoke(std::forward<Fun>(fun), std::forward<Obj>(obj), std::forward<Args>(args)..., api_callback);
	}

	/**
	 * @brief Construct an async object wrapping an invokeable object, the call is made immediately by forwarding to <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a> and can be awaited later to retrieve the result.
	 *
	 * @param fun The object to call using <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a>. Its last parameter must be a callable taking a parameter of type R
	 * @param args Parameters to pass to the object, excluding the callback
	 */
	template <typename Fun, typename... Args>
#ifndef _DOXYGEN_
	requires std::invocable<Fun, Args..., std::function<void(R)>>
#endif
	explicit async(Fun &&fun, Args&&... args) : async{std::make_shared<basic_promise<R>>()} {
		std::invoke(std::forward<Fun>(fun), std::forward<Args>(args)..., api_callback);
	}

	async(const async&) = delete;
	async(async&&) = default;

	async& operator=(const async&) = delete;
	async& operator=(async&&) = default;

	~async() {
		this->abandon();
	}
};

DPP_CHECK_ABI_COMPAT(async<>, async_dummy);

} // namespace dpp

#endif /* DPP_CORO */
