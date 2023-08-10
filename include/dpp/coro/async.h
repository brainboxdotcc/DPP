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

#include <mutex>
#include <utility>
#include <type_traits>
#include <functional>

namespace dpp {

struct confirmation_callback_t;

/**
 * @brief A co_await-able object handling an API call in parallel with the caller.
 *
 * This class is the return type of the dpp::cluster::co_* methods, but it can also be created manually to wrap any async call.
 *
 * @remark - This object's methods, other than constructors and operators, should not be called directly. It is designed to be used with coroutine keywords such as co_await.
 * @remark - This object must not be co_await-ed more than once.
 * @remark - The coroutine may be resumed in another thread, do not rely on thread_local variables.
 * @warning This feature is EXPERIMENTAL. The API may change at any time and there may be bugs. Please report any to <a href="https://github.com/brainboxdotcc/DPP/issues">GitHub issues</a> or to the <a href="https://discord.gg/dpp">D++ Discord server</a>.
 * @tparam R The return type of the API call. Defaults to confirmation_callback_t
 */
template <typename R>
class async {
	/**
	 * @brief Ref-counted callback, contains the callback logic and manages the lifetime of the callback data over multiple threads.
	 */
	struct shared_callback {
		/**
		 * @brief State of the async and its callback.
		 */
		struct callback_state {
			enum state_t {
				waiting,
				done,
				dangling
			};

			/**
			 * @brief Mutex to ensure the API result isn't set at the same time the coroutine is awaited and its value is checked, or the async is destroyed
			 */
			std::mutex mutex{};

			/**
			 * @brief Number of references to this callback state.
			 */
			int ref_count;

			/**
			 * @brief State of the awaitable and the API callback
			 */
			state_t state = waiting;

			/**
			 * @brief The stored result of the API call
			 */
			std::optional<R> result = std::nullopt;

			/**
			 * @brief Handle to the coroutine co_await-ing on this API call
			 *
			 * @see <a href="https://en.cppreference.com/w/cpp/coroutine/coroutine_handle">std::coroutine_handle</a>
			 */
			detail::std_coroutine::coroutine_handle<> coro_handle = nullptr;
		};

		callback_state *state;

		/**
		 * @brief Callback function.
		 *
		 * @param cback The result of the API call.
		 */
		void operator()(const R &cback) const {
			std::unique_lock lock{get_mutex()};

			if (state->state == callback_state::dangling) // Async object is gone - likely an exception killed it or it was never co_await-ed
				return;
			state->result = cback;
			state->state = callback_state::done;
			if (state->coro_handle) {
				auto handle = state->coro_handle;
				state->coro_handle = nullptr;
				lock.unlock();
				handle.resume();
			}
		}

		/**
		 * @brief Main constructor, allocates a new callback_state object.
		 */
		shared_callback() : state{new callback_state{.ref_count = 1}} {}

		/**
		 * @brief Destructor. Releases the held reference and destroys if no other references exist.
		 */
		~shared_callback() {
			if (!state) // Moved-from object
				return;

			std::unique_lock lock{state->mutex};

			if (state->ref_count) {
				--(state->ref_count);
				if (state->ref_count <= 0) {;
					lock.unlock();
					delete state;
				}
			}
		}

		/**
		 * @brief Copy constructor. Takes shared ownership of the callback state, increasing the reference count.
		 */
		shared_callback(const shared_callback &other) {
			this->operator=(other);
		}

		/**
		 * @brief Move constructor. Transfers ownership from another object, leaving intact the reference count. The other object releases the callback state.
		 */
		shared_callback(shared_callback &&other) noexcept {
			this->operator=(std::move(other));
		}

		/**
		 * @brief Copy assignment. Takes shared ownership of the callback state, increasing the reference count.
		 */
		shared_callback &operator=(const shared_callback &other) noexcept {
			std::lock_guard lock{other.get_mutex()};

			state = other.state;
			++state->ref_count;
			return *this;
		}

		/**
		 * @brief Move assignment. Transfers ownership from another object, leaving intact the reference count. The other object releases the callback state.
		 */
		shared_callback &operator=(shared_callback &&other) noexcept {
			std::lock_guard lock{other.get_mutex()};

			state = std::exchange(other.state, nullptr);
			return *this;
		}

		/**
		 * @brief Function called by the async when it is destroyed when it was never co_awaited, signals to the callback to abort.
		 */
		void set_dangling() {
			if (!state) // moved-from object
				return;
			std::lock_guard lock{get_mutex()};

			if (state->state == callback_state::waiting)
				state->state = callback_state::dangling;
		}

		/**
		 * @brief Convenience function to get the shared callback state's mutex.
		 */
		std::mutex &get_mutex() const {
			return (state->mutex);
		}

		/**
		 * @brief Convenience function to get the shared callback state's result.
		 */
		std::optional<R> &get_result() const {
			return (state->result);
		}
	};

	/**
	 * @brief Shared state of the async and its callback, to be used across threads.
	 */
	shared_callback api_callback;

public:
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
	async(Obj &&obj, Fun &&fun, Args&&... args) : api_callback{} {
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
	async(Fun &&fun, Args&&... args) : api_callback{} {
		std::invoke(std::forward<Fun>(fun), std::forward<Args>(args)..., api_callback);
	}

	/**
	 * @brief Construct an async wrapping an awaitable, the call is made immediately by forwarding to <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a> and can be awaited later to retrieve the result.
	 *
	 * @param callable The awaitable object whose API call to execute.
	 */
	async(const awaitable<R> &awaitable) : api_callback{} {
		std::invoke(awaitable.request, api_callback);
	}

	/**
	 * @brief Destructor. If any callback is pending it will be aborted.
	 */
	~async() {
		api_callback.set_dangling();
	}

	/**
	 * @brief Copy constructor is disabled
	 */
	async(const async &) = delete;

	/**
	 * @brief Move constructor
	 *
	 * NOTE: Despite being marked noexcept, this function uses std::lock_guard which may throw. The implementation assumes this can never happen, hence noexcept. Report it if it does, as that would be a bug.
	 *
	 * @remark Using the moved-from async after this function is undefined behavior.
	 * @param other The async object to move the data from.
	 */
	async(async &&other) noexcept = default;

	/**
	 * @brief Copy assignment is disabled
	 */
	async &operator=(const async &) = delete;

	/**
	 * @brief Move assignment operator.
	 *
	 * NOTE: Despite being marked noexcept, this function uses std::lock_guard which may throw. The implementation assumes this can never happen, hence noexcept. Report it if it does, as that would be a bug.
	 *
	 * @remark Using the moved-from async after this function is undefined behavior.
	 * @param other The async object to move the data from
	 */
	async &operator=(async &&other) noexcept = default;

	/**
	 * @brief First function called by the standard library when the object is co-awaited.
	 *
	 * Returns whether we already have the result of the API call and don't need to suspend the caller.
	 *
	 * @remark Do not call this manually, use the co_await keyword instead.
	 * @return bool Whether we already have the result of the API call or not
	 */
	bool await_ready() noexcept {
		std::lock_guard lock{api_callback.get_mutex()};

		return api_callback.get_result().has_value();
	}

	/**
	 * @brief Second function called by the standard library when the object is co-awaited, if await_ready returned false.
	 *
	 * Checks again for the presence of the result, if absent, signals to suspend and keep track of the calling coroutine for the callback to resume.
	 *
	 * @remark Do not call this manually, use the co_await keyword instead.
	 * @param handle The handle to the coroutine co_await-ing and being suspended
	 */
	template <typename T>
	bool await_suspend(detail::std_coroutine::coroutine_handle<T> caller) {
		std::lock_guard lock{api_callback.get_mutex()};

		if (api_callback.get_result().has_value())
			return false; // immediately resume the coroutine as we already have the result of the api call
		if constexpr (requires (T t) { t.is_sync = false; })
			caller.promise().is_sync = false;
		api_callback.state->coro_handle = caller;
		return true; // suspend the caller, the callback will resume it
	}

	/**
	 * @brief Function called by the standard library when the async is resumed. Its return value is what the whole co_await expression evaluates to
	 *
	 * @remark Do not call this manually, use the co_await keyword instead.
	 * @return R The result of the API call.
	 */
	R await_resume() {
		// no locking needed here as the callback has already executed
		return std::move(*api_callback.get_result());
	}
};

} // namespace dpp

#endif /* DPP_CORO */
