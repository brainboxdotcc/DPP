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

struct task_dummy : awaitable_dummy {
	int* handle_dummy = nullptr;
};

}

#ifdef DPP_CORO

#include <dpp/coro/coro.h>

#include <utility>
#include <type_traits>
#include <optional>
#include <functional>
#include <mutex>
#include <exception>
#include <atomic>

#include <iostream> // std::cerr in final_suspend

namespace dpp {

namespace detail {

/* Internal cogwheels for dpp::task */
namespace task {

/**
 * @brief A @ref dpp::task "task"'s promise_t type, with special logic for handling nested tasks.
 *
 * @tparam R Return type of the task
 */
template <typename R>
struct promise_t;

/**
 * @brief The object automatically co_await-ed at the end of a @ref dpp::task "task". Ensures nested coroutine chains are resolved, and the promise_t cleans up if it needs to.
 *
 * @tparam R Return type of the task
 */
template <typename R>
struct final_awaiter;

/**
 * @brief Alias for <a href="https://en.cppreference.com/w/cpp/coroutine/coroutine_handle"std::coroutine_handle</a> for a @ref dpp::task "task"'s @ref promise_t.
 *
 * @tparam R Return type of the task
 */
template <typename R>
using handle_t = std_coroutine::coroutine_handle<promise_t<R>>;

} // namespace task

} // namespace detail

/**
 * @class task task.h coro/task.h
 * @brief A coroutine task. It starts immediately on construction and can be co_await-ed, making it perfect for parallel coroutines returning a value.
 *
 * @warning - This feature is EXPERIMENTAL. The API may change at any time and there may be bugs.
 * Please report any to <a href="https://github.com/brainboxdotcc/DPP/issues">GitHub Issues</a> or to our <a href="https://discord.gg/dpp">Discord Server</a>.
 * @tparam R Return type of the task. Cannot be a reference but can be void.
 */
template <typename R>
#ifndef _DOXYGEN_
requires (!std::is_reference_v<R>)
#endif
class [[nodiscard("dpp::task cancels itself on destruction. use co_await on it, or its sync_wait method")]] task : public awaitable<R> {
	friend struct detail::task::promise_t<R>;

	using handle_t = detail::task::handle_t<R>;
	using state_flags = detail::promise::state_flags;

	handle_t handle{};

protected:
	/**
	 * @brief Construct from a coroutine handle. Internal use only
	 */
	explicit task(handle_t handle_) : awaitable<R>(&handle_.promise()), handle(handle_) {}

	/**
	 * @brief Clean up our handle, cancelling any running task
	 */
	void cleanup() {
		if (handle && this->valid()) {
			if (this->abandon() & state_flags::sf_done) {
				handle.destroy();
			} else {
				cancel();
			}
			handle = nullptr;
		}
	}

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
	task(task &&other) noexcept : awaitable<R>(std::move(other)), handle(std::exchange(other.handle, nullptr)) {}

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
		cleanup();
		handle = std::exchange(other.handle, nullptr);
		awaitable<R>::operator=(std::move(other));
		return *this;
	}

	/**
	 * @brief Destructor.
	 *
	 * Destroys the handle. If the task is still running, it will be cancelled.
	 */
	~task() {
		cleanup();
	}

	/**
	 * @brief Function to check if the task has finished its execution entirely
	 *
	 * @return bool Whether the task is finished.
	 */
	[[nodiscard]] bool done() const noexcept {
		return handle && (!this->valid() || handle.promise().state.load(std::memory_order_relaxed) == state_flags::sf_done);
	}

	/**
	 * @brief Cancel the task, it will stop the next time it uses co_await. On co_await-ing this task, throws dpp::task_cancelled_exception.
	 *
	 * @return *this
	 */
	task& cancel() & noexcept {
		handle.promise().cancelled.exchange(true, std::memory_order_relaxed);
		return *this;
	}

	/**
	 * @brief Cancel the task, it will stop the next time it uses co_await. On co_await-ing this task, throws dpp::task_cancelled_exception.
	 *
	 * @return *this
	 */
	task&& cancel() && noexcept {
		handle.promise().cancelled.exchange(true, std::memory_order_relaxed);
		return *this;
	}
};

namespace detail::task {
/**
 * @brief Awaitable returned from task::promise_t's final_suspend. Resumes the parent and cleans up its handle if needed
 */
template <typename R>
struct final_awaiter {
	/**
	 * @brief Always suspend at the end of the task. This allows us to clean up and resume the parent
	 */
	[[nodiscard]] bool await_ready() const noexcept {
		return (false);
	}

	/**
	 * @brief The suspension logic of the coroutine when it finishes. Always suspend the caller, meaning cleaning up the handle is on us
	 *
	 * @param handle The handle of this coroutine
	 * @return std::coroutine_handle<> Handle to resume, which is either the parent if present or std::noop_coroutine() otherwise
	 */
	[[nodiscard]] std_coroutine::coroutine_handle<> await_suspend(handle_t<R> handle) const noexcept;

	/**
	 * @brief Function called when this object is co_awaited by the standard library at the end of final_suspend. Do nothing, return nothing
	 */
	void await_resume() const noexcept {}
};

/**
 * @brief Base implementation of task::promise_t, without the logic that would depend on the return type. Meant to be inherited from
 */
template <typename R>
struct promise_base : basic_promise<R> {
	/**
	 * @brief Whether the task is cancelled or not.
	 */
	std::atomic<bool> cancelled = false;

#ifdef DPP_CORO_TEST
	promise_base() {
		++coro_alloc_count<promise_base>;
	}

	~promise_base() {
		--coro_alloc_count<promise_base>;
	}
#endif

	/**
	 * @brief Function called by the standard library when the coroutine is created.
	 *
	 * @return <a href="https://en.cppreference.com/w/cpp/coroutine/suspend_never">std::suspend_never</a> Don't suspend, the coroutine starts immediately.
	 */
	[[nodiscard]] std_coroutine::suspend_never initial_suspend() const noexcept {
		return {};
	}

	/**
	 * @brief Function called by the standard library when an exception is thrown and not caught in the coroutine.
	 *
	 * Stores the exception pointer to rethrow on co_await. If the task object is destroyed and was not cancelled, throw instead
	 */
	void unhandled_exception() {
		if ((this->state.load() & promise::state_flags::sf_broken) && !cancelled) {
			throw;
		}
		this->template set_exception<false>(std::current_exception());
	}

	/**
	 * @brief Proxy awaitable that wraps any co_await inside the task and checks for cancellation on resumption
	 *
	 * @see await_transform
	 */
	template <typename A>
	struct proxy_awaiter {
		/** @brief The promise_t object bound to this proxy */
		const promise_base &promise;

		/** @brief The inner awaitable being awaited */
		A awaitable;

		/** @brief Wrapper for the awaitable's await_ready */
		[[nodiscard]] bool await_ready() noexcept(noexcept(awaitable.await_ready())) {
			return awaitable.await_ready();
		}

		/** @brief Wrapper for the awaitable's await_suspend */
		template <typename T>
		[[nodiscard]] decltype(auto) await_suspend(T&& handle) noexcept(noexcept(awaitable.await_suspend(std::forward<T>(handle)))) {
			return awaitable.await_suspend(std::forward<T>(handle));
		}

		/**
		 * @brief Wrapper for the awaitable's await_resume, throws if the task is cancelled
		 *
		 * @throw dpp::task_cancelled_exception If the task was cancelled
		 */
		decltype(auto) await_resume() {
			if (promise.cancelled.load()) {
				throw dpp::task_cancelled_exception{"task was cancelled"};
			}
			return awaitable.await_resume();
		}
	};

	/**
	 * @brief Function called whenever co_await is used inside of the task
	 *
	 * @throw dpp::task_cancelled_exception On resumption if the task was cancelled
	 *
	 * @return @ref proxy_awaiter Returns a proxy awaiter that will check for cancellation on resumption
	 */
	template <awaitable_type T>
	[[nodiscard]] auto await_transform(T&& expr) const noexcept(noexcept(co_await_resolve(std::forward<T>(expr)))) {
		using awaitable_t = decltype(co_await_resolve(std::forward<T>(expr)));
		return proxy_awaiter<awaitable_t>{*this, co_await_resolve(std::forward<T>(expr))};
	}
};

/**
 * @brief Implementation of task::promise_t for non-void return type
 */
template <typename R>
struct promise_t : promise_base<R> {
	friend struct final_awaiter<R>;

	/**
	 * @brief Function called by the standard library when the coroutine co_returns a value.
	 *
	 * Stores the value internally to hand to the caller when it resumes.
	 *
	 * @param expr The value given to co_return
	 */
	void return_value(R&& expr) noexcept(std::is_nothrow_move_constructible_v<R>) requires std::move_constructible<R> {
		this->template set_value<false>(std::move(expr));
	}

	/**
	 * @brief Function called by the standard library when the coroutine co_returns a value.
	 *
	 * Stores the value internally to hand to the caller when it resumes.
	 *
	 * @param expr The value given to co_return
	 */
	void return_value(const R &expr) noexcept(std::is_nothrow_copy_constructible_v<R>) requires std::copy_constructible<R> {
		this->template set_value<false>(expr);
	}

	/**
	 * @brief Function called by the standard library when the coroutine co_returns a value.
	 *
	 * Stores the value internally to hand to the caller when it resumes.
	 *
	 * @param expr The value given to co_return
	 */
	template <typename T>
	requires (!std::is_same_v<R, std::remove_cvref_t<T>> && std::convertible_to<T, R>)
	void return_value(T&& expr) noexcept (std::is_nothrow_convertible_v<T, R>) {
		this->template emplace_value<false>(std::forward<T>(expr));
	}

	/**
	 * @brief Function called by the standard library when the coroutine is created.
	 *
	 * @return dpp::task The coroutine object
	 */
	[[nodiscard]] dpp::task<R> get_return_object() noexcept {
		return dpp::task<R>{handle_t<R>::from_promise(*this)};
	}

	/**
	 * @brief Function called by the standard library when the coroutine reaches its last suspension point
	 *
	 * @return final_awaiter Special object containing the chain resolution and clean-up logic.
	 */
	[[nodiscard]] final_awaiter<R> final_suspend() const noexcept {
		return {};
	}
};

/**
 * @brief Implementation of task::promise_t for void return type
 */
template <>
struct promise_t<void> : promise_base<void> {
	friend struct final_awaiter<void>;

	/**
	 * @brief Function called by the standard library when the coroutine co_returns
	 *
	 * Sets the promise state to finished.
	 */
	void return_void() noexcept {
		set_value<false>();
	}

	/**
	 * @brief Function called by the standard library when the coroutine is created.
	 *
	 * @return task The coroutine object
	 */
	[[nodiscard]] dpp::task<void> get_return_object() noexcept {
		return dpp::task<void>{handle_t<void>::from_promise(*this)};
	}

	/**
	 * @brief Function called by the standard library when the coroutine reaches its last suspension point
	 *
	 * @return final_awaiter Special object containing the chain resolution and clean-up logic.
	 */
	[[nodiscard]] final_awaiter<void> final_suspend() const noexcept {
		return {};
	}
};

template <typename R>
std_coroutine::coroutine_handle<> final_awaiter<R>::await_suspend(handle_t<R> handle) const noexcept {
	using state_flags = promise::state_flags;
	promise_t<R> &promise = handle.promise();
	uint8_t previous_state = promise.state.fetch_or(state_flags::sf_done);

	if ((previous_state & state_flags::sf_awaited) != 0) { // co_await-ed, resume parent
		if ((previous_state & state_flags::sf_broken) != 0) { // major bug, these should never be set together
			// we don't have a cluster so just log it on cerr
			std::cerr << "dpp: task promise ended in both an awaited and dangling state. this is a bug and a memory leak, please report it to us!" << std::endl;
		}
		return promise.release_awaiter();
	}
	if ((previous_state & state_flags::sf_broken) != 0) { // task object is gone, free the handle
		handle.destroy();
	}
	return std_coroutine::noop_coroutine();
}

} // namespace detail::task

DPP_CHECK_ABI_COMPAT(task<void>, task_dummy)
DPP_CHECK_ABI_COMPAT(task<uint64_t>, task_dummy)

} // namespace dpp

/**
 * @brief Specialization of std::coroutine_traits, helps the standard library figure out a promise_t type from a coroutine function.
 */
template<typename T, typename... Args>
struct dpp::detail::std_coroutine::coroutine_traits<dpp::task<T>, Args...> {
	using promise_type = dpp::detail::task::promise_t<T>;
};

#endif /* DPP_CORO */
