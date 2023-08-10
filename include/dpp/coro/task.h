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

#include <utility>
#include <type_traits>
#include <optional>
#include <functional>
#include <mutex>
#include <exception>

namespace dpp {

namespace detail {

/**
 * @brief A task's promise type, with special logic for handling nested tasks.
 */
template <typename R>
struct task_promise;

/**
 * @brief The object automatically co_await-ed at the end of a task. Ensures nested task chains are resolved, and the promise cleans up if it needs to.
 */
template <typename R>
struct task_chain_final_awaiter;

/**
 * @brief Alias for <a href="https://en.cppreference.com/w/cpp/coroutine/coroutine_handle">std::coroutine_handle</a> for a task_promise.
 */
template <typename R>
using task_handle = detail::std_coroutine::coroutine_handle<detail::task_promise<R>>;

} // namespace detail

/**
 * @brief A coroutine task. It can be co_awaited to make nested coroutines.
 *
 * Can be used in conjunction with coroutine events via dpp::event_router_t::co_attach, or on its own.
 *
 * @warning This feature is EXPERIMENTAL. The API may change at any time and there may be bugs. Please report any to <a href="https://github.com/brainboxdotcc/DPP/issues">GitHub issues</a> or to the <a href="https://discord.gg/dpp">D++ Discord server</a>.
 * @tparam R Return type of the coroutine. Can be void, or a complete object that supports move construction and move assignment.
 */
template <typename R>
#ifndef _DOXYGEN_
requires std::is_same_v<void, R> || (!std::is_reference_v<R> && std::is_move_constructible_v<R> && std::is_move_assignable_v<R>)
#endif
class task {
	/**
	 * @brief The coroutine handle of this task.
	 */
	detail::task_handle<R> handle;

	/**
	 * @brief Promise type of this coroutine. For internal use only, do not use.
	 */
	friend struct detail::task_promise<R>;

	/**
	 * @brief Construct from a coroutine handle. Internal use only
	 */
	explicit task(detail::task_handle<R> handle_) : handle(handle_) {}

public:
	/**
	 * @brief Default constructor, creates a task not bound to a coroutine.
	 */
	task() = default;

	/**
	 * @brief Copy constructor is disabled
	 */
	task(const task &) = delete;

	/**
	 * @brief Move constructor, grabs another task's coroutine handle
	 *
	 * @param other Task to move the handle from
	 */
	task(task &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}


	/**
	 * @brief Destructor.
	 *
	 * Destroys the handle if coroutine is done, otherwise detaches it from this thread.
	 * In detached mode, the handle will destroy itself at the end of the coroutine.
	 */
	~task() {
		if (handle) {
			auto &promise = handle.promise();

			if (!promise.is_sync) {
				std::unique_lock lock{promise.mutex};

				if (promise.destroy) // promise in async thread checked first and skipped clean up, we do it
				{
					if (promise.exception && promise.exception_handler)
						promise.exception_handler(promise.exception);
					lock.unlock();
					handle.destroy();
				}
				else
					handle.promise().destroy = true;
			}
			else {
				if (promise.exception && promise.exception_handler)
					promise.exception_handler(promise.exception);
				handle.destroy();
			}
		}
	}

	/**
	 * @brief Copy assignment is disabled
	 */
	task &operator=(const task &) = delete;

	/**
	 * @brief Move assignment, grabs another task's coroutine handle
	 *
	 * @param other Task to move the handle from
	 */
	task &operator=(task &&other) noexcept {
		handle = std::exchange(other.handle, nullptr);
		return (*this);
	}

	/**
	 * @brief First function called by the standard library when the task is co_await-ed.
	 *
	 * @remark Do not call this manually, use the co_await keyword instead.
	 * @return bool Whether not to suspend the caller or not
	 */
	bool await_ready() {
		return handle.promise().is_sync;
	}

	/**
	 * @brief Second function called by the standard library when the task is co_await-ed, if await_ready returned false.
	 *
	 * Stores the calling coroutine in the promise to resume when this task suspends.
	 *
	 * @remark Do not call this manually, use the co_await keyword instead.
	 * @param caller The calling coroutine, now suspended
	 * @return bool Whether to suspend the caller or not
	 */
	template <typename T>
	bool await_suspend(detail::std_coroutine::coroutine_handle<T> caller) {
		auto &my_promise = handle.promise();

		if (my_promise.is_sync)
			return false;

		std::lock_guard lock{my_promise.mutex};

		if (handle.done())
			return (false);
		if constexpr (requires (T t) { t.is_sync = false; })
			caller.promise().is_sync = false;
		my_promise.parent = caller;
		return true;
	}

	/**
	 * @brief Function called by the standard library when the coroutine is resumed.
	 *
	 * @remark Do not call this manually, use the co_await keyword instead.
	 * @throw Throws any exception thrown or uncaught by the coroutine
	 * @return R The result of the coroutine. It is the value the whole co-await expression evaluates to
	 */
	R await_resume();

	/**
	 * @brief Function to check if the coroutine has finished its execution entirely
	 *
	 * @return bool Whether the coroutine is done.
	 * @see https://en.cppreference.com/w/cpp/coroutine/coroutine_handle/done
	 */
	bool done() const noexcept {
		return handle.done();
	}

	/**
	 * @brief Set the exception handling function. Called when an exception is thrown but not caught
	 *
	 * @warning The exception handler must not throw. If an exception that is not caught is thrown in a detached task, the program will terminate.
	 */
	task &on_exception(std::function<void(std::exception_ptr)> func) {
		handle.promise().exception_handler = std::move(func);
		if (handle.promise().exception)
			func(handle.promise().exception);
		return *this;
	}
};

namespace detail {
	/**
 * @brief Awaitable returned from task_promise's final_suspend. Resumes the parent and cleans up its handle if needed
 */
template <typename R>
struct task_chain_final_awaiter {
	/**
	 * @brief Always suspend at the end of the task. This allows us to clean up and resume the parent
	 */
	bool await_ready() noexcept {
		return (false);
	}

	/*
	 * @brief The suspension logic of the coroutine when it finishes. Always suspend the caller, meaning cleaning up the handle is on us
	 *
	 * @param handle The handle of this coroutine
	 */
	void await_suspend(detail::task_handle<R> handle) noexcept;

	/*
	 * @brief Function called when this object is co_awaited by the standard library at the end of final_suspend. Do nothing, return nothing
	 */
	void await_resume() noexcept {}
};
/**
 * @brief Base implementation of task_promise, without the logic that would depend on the return type. Meant to be inherited from
 */
struct task_promise_base {
	/**
	 * @brief Mutex for async task destruction.
	 */
	std::mutex mutex{};

	/**
	 * @brief Parent coroutine to return to for nested coroutines.
	 */
	detail::std_coroutine::coroutine_handle<> parent = nullptr;

	/**
	 * @brief Exception ptr if any was thrown during the coroutine
	 *
	 * @see <a href="https://en.cppreference.com/w/cpp/error/exception_ptr">std::exception_ptr</a>
	 */
	std::exception_ptr exception = nullptr;

	/**
	 * @brief Whether the coroutine has async calls or not
	 *
	 * Will only ever change on the calling thread while callback mutex guards the async thread
	 */
	bool is_sync = true;

	/**
	 * @brief Whether either the task object or the promise is gone and the next one to end will clean up
	 */
	bool destroy = false;

	/**
	 * @brief Function object called when an exception is thrown from a coroutine
	 */
	std::function<void(std::exception_ptr)> exception_handler = nullptr;

	/**
	 * @brief Function called by the standard library when the coroutine is created.
	 *
	 * @return <a href="https://en.cppreference.com/w/cpp/coroutine/suspend_never">std::suspend_never</a> Don't suspend, the coroutine starts immediately.
	 */
	std_coroutine::suspend_never initial_suspend() noexcept {
		return {};
	}

	/**
	 * @brief Function called by the standard library when an exception is thrown and not caught in the coroutine.
	 *
	 * Stores the exception pointer to rethrow later
	 */
	void unhandled_exception() {
		exception = std::current_exception();
	}
};

/**
 * @brief Implementation of task_promise for non-void return type
 */
template <typename R>
struct task_promise : task_promise_base {
	/**
	 * @brief Stored return value of the coroutine.
	 *
	 * @details The main reason we use std::optional<R> here and not R is to avoid default construction of the value so we only require R to have a move constructor, instead of both a default constructor and move assignment operator
	 */
	std::optional<R> value = std::nullopt;

	/**
	 * @brief Function called by the standard library when the coroutine co_returns a value.
	 *
	 * Stores the value internally to hand to the caller when it resumes.
	 *
	 * @param expr The value given to co_return
	 */
	void return_value(R expr) {
		value = std::move(expr);
	}

	/**
	 * @brief Function called by the standard library when the coroutine is created.
	 *
	 * @return task The coroutine object
	 */
	task<R> get_return_object() {
		return task{task_handle<R>::from_promise(*this)};
	}

	/**
	 * @brief Function called by the standard library when the coroutine reaches its last suspension point
	 *
	 * @return task_chain_final_awaiter Special object containing the chain resolution and clean-up logic.
	 */
	task_chain_final_awaiter<R> final_suspend() noexcept {
		return {};
	}
};

/**
 * @brief Implementation of task_promise for void return type
 */
template <>
struct task_promise<void> : task_promise_base {
	/**
	 * @brief Function called by the standard library when the coroutine co_returns
	 *
	 * Does nothing but is required by the standard library.
	 */
	void return_void() {}

	/**
	 * @brief Function called by the standard library when the coroutine is created.
	 *
	 * @return task The coroutine object
	 */
	task<void> get_return_object() {
		return task<void>{task_handle<void>::from_promise(*this)};
	}

	/**
	 * @brief Function called by the standard library when the coroutine reaches its last suspension point
	 *
	 * @return task_chain_final_awaiter Special object containing the chain resolution and clean-up logic.
	 */
	task_chain_final_awaiter<void> final_suspend() noexcept {
		return {};
	}
};

template <typename R>
void detail::task_chain_final_awaiter<R>::await_suspend(detail::task_handle<R> handle) noexcept {
	task_promise<R> &promise = handle.promise();
	std_coroutine::coroutine_handle<> parent = promise.parent;

	if (!promise.is_sync) {
		std::unique_lock lock{promise.mutex};

		if (promise.destroy) {
			if (promise.exception && promise.exception_handler)
				promise.exception_handler(promise.exception);
			lock.unlock();
			handle.destroy();
		}
		else
			promise.destroy = true; // Send the destruction back to the task
	}
	if (parent)
		parent.resume();
}

} // namespace detail

template <typename R>
#ifndef _DOXYGEN_
requires std::is_same_v<void, R> || (!std::is_reference_v<R> && std::is_move_constructible_v<R> && std::is_move_assignable_v<R>)
#endif
R task<R>::await_resume() {
	if (handle.promise().exception) // If we have an exception, rethrow
		std::rethrow_exception(handle.promise().exception);
	if constexpr (!std::is_same_v<R, void>) // If we have a return type, return it and clean up our stored value
		return *std::exchange(handle.promise().value, std::nullopt);
}

} // namespace dpp

/**
 * @brief Specialization of std::coroutine_traits, helps the standard library figure out a promise type from a coroutine function.
 */
template<typename T, typename... Args>
struct dpp::detail::std_coroutine::coroutine_traits<dpp::task<T>, Args...> {
	using promise_type = dpp::detail::task_promise<T>;
};

#endif /* DPP_CORO */
