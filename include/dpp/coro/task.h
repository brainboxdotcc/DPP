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
#include <atomic>

#include <iostream> // std::cerr in final_suspend

namespace dpp {

namespace detail {

enum class task_state_t {
	/** @brief Task was started but never co_await-ed */
	started,
	/** @brief Task was co_await-ed and is pending completion */
	awaited,
	/** @brief Task is completed */
	done,
	/** @brief Task is still running but the actual dpp::task object is destroyed */
	dangling
};

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

/**
 * @brief Base class of dpp::task<R>.
 *
 * @warning This class should not be used directly by a user, use dpp::task<R> instead.
 * @note This class contains all the functions used internally by co_await. It is intentionally opaque and a private base of dpp::task<R> so a user cannot call await_suspend and await_resume directly.
 */
template <typename R>
class task_base {
protected:
	/**
	 * @brief The coroutine handle of this task.
	 */
	detail::task_handle<R> handle;

	/**
	 * @brief Promise type of this coroutine. For internal use only, do not use.
	 */
	friend struct detail::task_promise<R>;

private:
	/**
	 * @brief Construct from a coroutine handle. Internal use only
	 */
	explicit task_base(detail::task_handle<R> handle_) : handle(handle_) {}

public:
	/**
	 * @brief Default constructor, creates a task not bound to a coroutine.
	 */
	task_base() = default;

	/**
	 * @brief Copy constructor is disabled
	 */
	task_base(const task_base &) = delete;

	/**
	 * @brief Move constructor, grabs another task's coroutine handle
	 *
	 * @param other Task to move the handle from
	 */
	task_base(task_base &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

	/**
	 * @brief Destructor.
	 *
	 * Destroys the handle.
	 * @warning The coroutine must be finished before this is called, otherwise it runs the risk of being resumed after it is destroyed, resuming in use-after-free undefined behavior.
	 */
	~task_base() {
		if (handle) {
			detail::task_promise<R> &promise = handle.promise();
			detail::task_state_t previous_state = promise.state.exchange(detail::task_state_t::dangling);

			if (previous_state == detail::task_state_t::done)
				handle.destroy();
			else
				cancel();
		}
	}

	/**
	 * @brief Copy assignment is disabled
	 */
	task_base &operator=(const task_base &) = delete;

	/**
	 * @brief Move assignment, grabs another task's coroutine handle
	 *
	 * @param other Task to move the handle from
	 */
	task_base &operator=(task_base &&other) noexcept {
		handle = std::exchange(other.handle, nullptr);
		return (*this);
	}

	/**
	 * @brief Check whether or not a call to co_await will suspend the caller.
	 *
	 * This function is called by the standard library as a first step when using co_await. If it returns true then the caller is not suspended.
	 * @throws logic_exception if the task is empty.
	 * @return bool Whether not to suspend the caller or not
	 */
	bool await_ready() const {
		if (!handle)
			throw dpp::logic_exception{"cannot co_await an empty task"};
		return handle.promise().state.load() == detail::task_state_t::done;
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
	bool await_suspend(detail::std_coroutine::coroutine_handle<> caller) {
		detail::task_promise<R> &my_promise = handle.promise();
		auto previous_state = detail::task_state_t::started;

		my_promise.parent = caller;
		// Replace `sent` state with `awaited` ; if that fails, the only logical option is the state was `done`, in which case return false to resume
		if (!handle.promise().state.compare_exchange_strong(previous_state, detail::task_state_t::awaited) && previous_state == detail::task_state_t::done)
			return false;
		return true;
	}

	/**
	 * @brief Function to check if the task has finished its execution entirely
	 *
	 * @return bool Whether the task is finished.
	 */
	[[nodiscard]] bool done() const noexcept {
		return handle && handle.promise().state.load(std::memory_order_relaxed) == detail::task_state_t::done;
	}

	/**
	 * @brief Cancel the task, it will stop the next time it uses co_await. On co_await-ing this task, throws dpp::task_cancelled_exception.
	 */
	dpp::task<R>& cancel() & noexcept {
		handle.promise().cancelled.exchange(true, std::memory_order_relaxed);
		return static_cast<task<R> &>(*this);
	}

	/**
	 * @brief Cancel the task, it will stop the next time it uses co_await. On co_await-ing this task, throws dpp::task_cancelled_exception.
	 */
	dpp::task<R>&& cancel() && noexcept {
		handle.promise().cancelled.exchange(true, std::memory_order_relaxed);
		return static_cast<task<R> &&>(*this);
	}

	/**
	 * @brief Function called by the standard library when resuming.
	 *
	 * @return R& Return value of the coroutine, handed to the caller of co_await.
	 */
	decltype(auto) await_resume() & {
		return static_cast<task<R> &>(*this).await_resume_impl();
	}

	/**
	 * @brief Function called by the standard library when resuming.
	 *
	 * @return const R& Return value of the coroutine, handed to the caller of co_await.
	 */
	decltype(auto) await_resume() const & {
		return static_cast<const task<R> &>(*this).await_resume_impl();
	}

	/**
	 * @brief Function called by the standard library when resuming.
	 *
	 * @return R&& Return value of the coroutine, handed to the caller of co_await.
	 */
	decltype(auto) await_resume() && {
		return static_cast<task<R> &&>(*this).await_resume_impl();
	}
};

} // namespace detail

/**
 * @brief A coroutine task. It starts immediately on construction and can be co_await-ed, making it perfect for parallel coroutines returning a value.
 *
 * Can be used in conjunction with coroutine events via dpp::event_router_t::co_attach, or on its own.
 *
 * @warning - This feature is EXPERIMENTAL. The API may change at any time and there may be bugs. Please report any to <a href="https://github.com/brainboxdotcc/DPP/issues">GitHub issues</a> or to the <a href="https://discord.gg/dpp">D++ Discord server</a>.
 * @tparam R Return type of the coroutine. Cannot be a reference, can be void.
 */
template <typename R>
#ifndef _DOXYGEN_
requires (!std::is_reference_v<R>)
#endif
class task : private detail::task_base<R> {
	/**
	 * @brief Internal use only base class containing common logic between task<R> and task<void>. It also serves to prevent await_suspend and await_resume from being used directly.
	 *
	 * @warning For internal use only, do not use.
	 * @see operator co_await()
	 */
	friend class detail::task_base<R>;

	/**
	 * @brief Function called by the standard library when the coroutine is resumed.
	 *
	 * @throw Throws any exception thrown or uncaught by the coroutine
	 * @return R& The result of the coroutine. This is returned to the awaiter as the result of co_await
	 */
	R& await_resume_impl() & {
		detail::task_promise<R> &promise = this->handle.promise();
		if (promise.exception)
			std::rethrow_exception(promise.exception);
		return *reinterpret_cast<R *>(promise.result_storage.data());
	}

	/**
	 * @brief Function called by the standard library when the coroutine is resumed.
	 *
	 * @throw Throws any exception thrown or uncaught by the coroutine
	 * @return const R& The result of the coroutine. This is returned to the awaiter as the result of co_await
	 */
	const R& await_resume_impl() const & {
		detail::task_promise<R> &promise = this->handle.promise();
		if (promise.exception)
			std::rethrow_exception(promise.exception);
		return *reinterpret_cast<R *>(promise.result_storage.data());
	}

	/**
	 * @brief Function called by the standard library when the coroutine is resumed.
	 *
	 * @throw Throws any exception thrown or uncaught by the coroutine
	 * @return R&& The result of the coroutine. This is returned to the awaiter as the result of co_await
	 */
	R&& await_resume_impl() && {
		detail::task_promise<R> &promise = this->handle.promise();
		if (promise.exception)
			std::rethrow_exception(promise.exception);
		return *reinterpret_cast<R *>(promise.result_storage.data());
	}

public:
#ifdef _DOXYGEN_ // :)
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
	task(task &&other) noexcept;

	/**
	 * @brief Destructor.
	 *
	 * Destroys the handle.
	 * @warning The coroutine must be finished before this is called, otherwise it runs the risk of being resumed after it is destroyed, resuming in use-after-free undefined behavior.
	 */
	~task();

	/**
	 * @brief Copy assignment is disabled
	 */
	task &operator=(const task &) = delete;

	/**
	 * @brief Move assignment, grabs another task's coroutine handle
	 *
	 * @param other Task to move the handle from
	 */
	task &operator=(task &&other) noexcept;

	/**
	 * @brief Function to check if the task has finished its execution entirely
	 *
	 * @return bool Whether the task is finished.
	 */
	[[nodiscard]] bool done() const noexcept;

	/**
	 * @brief Cancel the task, it will stop the next time it uses co_await. On co_await-ing this task, throws dpp::task_cancelled_exception.
	 */
	dpp::task<R>& cancel() & noexcept;

	/**
	 * @brief Check whether or not a call to co_await will suspend the caller.
	 *
	 * This function is called by the standard library as a first step when using co_await. If it returns true then the caller is not suspended.
	 * @throws logic_exception if the task is empty.
	 * @return bool Whether not to suspend the caller or not
	 */
	bool await_ready() const;
#else
	using detail::task_base<R>::task_base; // use task_base's constructors
	using detail::task_base<R>::operator=; // use task_base's assignment operators
	using detail::task_base<R>::done; // expose done() as public
	using detail::task_base<R>::cancel; // expose cancel() as public
	using detail::task_base<R>::await_ready; // expose await_ready as public
#endif

	/**
	 * @brief Suspend the current coroutine until the task completes.
	 *
	 * @throw On resumption, any exception thrown by the coroutine is propagated to the caller.
	 * @return R& On resumption, this expression evaluates to the result object of type R, as a reference.
	 */
	auto& operator co_await() & noexcept {
		return static_cast<detail::task_base<R>&>(*this);
	}

	/**
	 * @brief Suspend the current coroutine until the task completes.
	 *
	 * @throw On resumption, any exception thrown by the coroutine is propagated to the caller.
	 * @return const R& On resumption, this expression evaluates to the result object of type R, as a const reference.
	 */
	const auto& operator co_await() const & noexcept {
		return static_cast<const detail::task_base<R>&>(*this);
	}

	/**
	 * @brief Suspend the current coroutine until the task completes.
	 *
	 * @throw On resumption, any exception thrown by the coroutine is propagated to the caller.
	 * @return R&& On resumption, this expression evaluates to the result object of type R, as an rvalue reference.
	 */
	auto&& operator co_await() && noexcept {
		return static_cast<detail::task_base<R>&&>(*this);
	}
};

#ifndef _DOXYGEN_ // don't generate this on doxygen because `using` doesn't work and 2 copies of coroutine_base's docs is enough
/**
 * @brief A coroutine task. It starts immediately on construction and can be co_await-ed, making it perfect for parallel coroutines returning a value.
 *
 * Can be used in conjunction with coroutine events via dpp::event_router_t::co_attach, or on its own.
 *
 * @warning - This feature is EXPERIMENTAL. The API may change at any time and there may be bugs. Please report any to <a href="https://github.com/brainboxdotcc/DPP/issues">GitHub issues</a> or to the <a href="https://discord.gg/dpp">D++ Discord server</a>.
 * @tparam R Return type of the coroutine. Cannot be a reference, can be void.
 */
template <>
class task<void> : private detail::task_base<void> {
	/**
	 * @brief Private base class containing common logic between task<R> and task<void>. It also serves to prevent await_suspend and await_resume from being used directly.
	 *
	 * @see operator co_await()
	 */
	friend class detail::task_base<void>;

	/**
	 * @brief Function called by the standard library when the coroutine is resumed.
	 *
	 * @remark Do not call this manually, use the co_await keyword instead.
	 * @throw Throws any exception thrown or uncaught by the coroutine
	 */
	void await_resume_impl() const;

public:
	using detail::task_base<void>::task_base; // use task_base's constructors
	using detail::task_base<void>::operator=; // use task_base's assignment operators
	using detail::task_base<void>::done; // expose done() as public
	using detail::task_base<void>::cancel; // expose cancel() as public
	using detail::task_base<void>::await_ready; // expose await_ready as public

	/**
	 * @brief Suspend the current coroutine until the task completes.
	 *
	 * @throw On resumption, any exception thrown by the coroutine is propagated to the caller.
	 */
	auto& operator co_await() & {
		return static_cast<detail::task_base<void>&>(*this);
	}

	/**
	 * @brief Suspend the current coroutine until the task completes.
	 *
	 * @throw On resumption, any exception thrown by the coroutine is propagated to the caller.
	 */
	const auto& operator co_await() const & {
		return static_cast<const detail::task_base<void>&>(*this);
	}

	/**
	 * @brief Suspend the current coroutine until the task completes.
	 *
	 * @throw On resumption, any exception thrown by the coroutine is propagated to the caller.
	 */
	auto&& operator co_await() && {
		return static_cast<detail::task_base<void>&&>(*this);
	}
};
#endif /* _DOXYGEN_ */

namespace detail {
	/**
 * @brief Awaitable returned from task_promise's final_suspend. Resumes the parent and cleans up its handle if needed
 */
template <typename R>
struct task_chain_final_awaiter {
	/**
	 * @brief Always suspend at the end of the task. This allows us to clean up and resume the parent
	 */
	bool await_ready() const noexcept {
		return (false);
	}

	/*
	 * @brief The suspension logic of the coroutine when it finishes. Always suspend the caller, meaning cleaning up the handle is on us
	 *
	 * @param handle The handle of this coroutine
	 * @return std::coroutine_handle<> Handle to resume, which is either the parent if present or std::noop_coroutine() otherwise
	 */
	std_coroutine::coroutine_handle<> await_suspend(detail::task_handle<R> handle) const noexcept;

	/*
	 * @brief Function called when this object is co_awaited by the standard library at the end of final_suspend. Do nothing, return nothing
	 */
	void await_resume() const noexcept {}
};
/**
 * @brief Base implementation of task_promise, without the logic that would depend on the return type. Meant to be inherited from
 */
struct task_promise_base {
	/**
	 * @brief State of the task, used to keep track of lifetime and status
	 */
	std::atomic<task_state_t> state = task_state_t::started;

	/**
	 * @brief Whether the task is cancelled or not.
	 */
	std::atomic<bool> cancelled = false;

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

#ifdef DPP_CORO_TEST
	task_promise_base() {
		++coro_alloc_count<task_promise_base>;
	}

	~task_promise_base() {
		--coro_alloc_count<task_promise_base>;
	}
#endif

	/**
	 * @brief Function called by the standard library when the coroutine is created.
	 *
	 * @return <a href="https://en.cppreference.com/w/cpp/coroutine/suspend_never">std::suspend_never</a> Don't suspend, the coroutine starts immediately.
	 */
	std_coroutine::suspend_never initial_suspend() const noexcept {
		return {};
	}

	/**
	 * @brief Function called by the standard library when an exception is thrown and not caught in the coroutine.
	 *
	 * Stores the exception pointer to rethrow on co_await. If the task object is destroyed and was not cancelled, throw instead
	 */
	void unhandled_exception() {
		exception = std::current_exception();
		if (state.load() == task_state_t::dangling && !cancelled)
			throw;
	}

	template <typename A>
	struct proxy_awaiter {
		const task_promise_base &promise;
		A awaitable;

		bool await_ready() noexcept(noexcept(awaitable.await_ready())) {
			return awaitable.await_ready();
		}

		template <typename T>
		decltype(auto) await_suspend(T&& handle) noexcept(noexcept(awaitable.await_suspend(std::forward<T>(handle)))) {
			return awaitable.await_suspend(std::forward<T>(handle));
		}

		decltype(auto) await_resume() {
			if (promise.cancelled.load())
				throw dpp::task_cancelled_exception{"task was cancelled"};
			return awaitable.await_resume();
		}
	};

	template <typename T>
	auto await_transform(T&& expr) const noexcept(noexcept(co_await_resolve(std::forward<T>(expr)))) {
		using awaitable_t = decltype(co_await_resolve(std::forward<T>(expr)));
		return proxy_awaiter<awaitable_t>{*this, co_await_resolve(std::forward<T>(expr))};
	}
};

/**
 * @brief Implementation of task_promise for non-void return type
 */
template <typename R>
struct task_promise : task_promise_base {
	~task_promise() {
		if (state.load() == task_state_t::done && !exception)
			std::destroy_at(reinterpret_cast<R *>(result_storage.data()));
	}

	/**
	 * @brief Stored return value of the coroutine.
	 *
	 * @details The main reason we use std::optional<R> here and not R is to avoid default construction of the value so we only require R to have a move constructor, instead of both a default constructor and move assignment operator
	 */
	alignas(R) std::array<std::byte, sizeof(R)> result_storage;

	/**
	 * @brief Function called by the standard library when the coroutine co_returns a value.
	 *
	 * Stores the value internally to hand to the caller when it resumes.
	 *
	 * @param expr The value given to co_return
	 */
	void return_value(R&& expr) noexcept(std::is_nothrow_move_constructible_v<R>) requires std::move_constructible<R> {
		std::construct_at<R>(reinterpret_cast<R *>(result_storage.data()), static_cast<R&&>(expr));
	}

	/**
	 * @brief Function called by the standard library when the coroutine co_returns a value.
	 *
	 * Stores the value internally to hand to the caller when it resumes.
	 *
	 * @param expr The value given to co_return
	 */
	void return_value(const R &expr) noexcept(std::is_nothrow_copy_constructible_v<R>) requires std::copy_constructible<R> {
		std::construct_at<R>(reinterpret_cast<R *>(result_storage.data()), expr);
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
		std::construct_at<R>(reinterpret_cast<R *>(result_storage.data()), std::forward<T>(expr));
	}

	/**
	 * @brief Function called by the standard library when the coroutine is created.
	 *
	 * @return task The coroutine object
	 */
	task<R> get_return_object() noexcept {
		return task<R>{task_handle<R>::from_promise(*this)};
	}

	/**
	 * @brief Function called by the standard library when the coroutine reaches its last suspension point
	 *
	 * @return task_chain_final_awaiter Special object containing the chain resolution and clean-up logic.
	 */
	task_chain_final_awaiter<R> final_suspend() const noexcept {
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
	void return_void() const noexcept {}

	/**
	 * @brief Function called by the standard library when the coroutine is created.
	 *
	 * @return task The coroutine object
	 */
	task<void> get_return_object() noexcept {
		return task<void>{task_handle<void>::from_promise(*this)};
	}

	/**
	 * @brief Function called by the standard library when the coroutine reaches its last suspension point
	 *
	 * @return task_chain_final_awaiter Special object containing the chain resolution and clean-up logic.
	 */
	task_chain_final_awaiter<void> final_suspend() const noexcept {
		return {};
	}
};

template <typename R>
std_coroutine::coroutine_handle<> detail::task_chain_final_awaiter<R>::await_suspend(task_handle<R> handle) const noexcept {
	task_promise<R> &promise = handle.promise();
	task_state_t previous_state = promise.state.exchange(task_state_t::done);

	switch (previous_state) {
		case task_state_t::started: // started but never awaited, suspend
			return std_coroutine::noop_coroutine();
		case task_state_t::awaited: // co_await-ed, resume parent
			return promise.parent;
		case task_state_t::dangling: // task object is gone, free the handle
			handle.destroy();
			return std_coroutine::noop_coroutine();
		case task_state_t::done: // what
			// this should never happen. log it. we don't have a cluster so just write it on cerr
			std::cerr << "dpp::task: final_suspend called twice. something went very wrong here, please report to GitHub issues or the D++ Discord server" << std::endl;
	}
	// TODO: replace with __builtin_unreachable when we confirm this never happens with normal usage
	return std_coroutine::noop_coroutine();
}

} // namespace detail

#ifndef _DOXYGEN_
inline void task<void>::await_resume_impl() const {
	if (handle.promise().exception)
		std::rethrow_exception(handle.promise().exception);
}
#endif /* _DOXYGEN_ */

} // namespace dpp

/**
 * @brief Specialization of std::coroutine_traits, helps the standard library figure out a promise type from a coroutine function.
 */
template<typename T, typename... Args>
struct dpp::detail::std_coroutine::coroutine_traits<dpp::task<T>, Args...> {
	using promise_type = dpp::detail::task_promise<T>;
};

#endif /* DPP_CORO */
