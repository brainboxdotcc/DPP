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

#include <iostream>

#include <dpp/utility.h>

namespace dpp {

struct awaitable_dummy {
	int *promise_dummy = nullptr;
};

}

#ifdef DPP_CORO

#include <dpp/coro/coro.h>

// Do not include <coroutine> as coro.h includes <experimental/coroutine> or <coroutine> depending on clang version
#include <mutex>
#include <utility>
#include <type_traits>
#include <exception>
#include <atomic>

namespace dpp {

namespace detail::promise {

/**
 * @brief State of a promise
 */
enum state_flags {
	/**
	 * @brief Promise is empty
	 */
	sf_none = 0b0000000,

	/**
	 * @brief Promise has spawned an awaitable
	 */
	sf_has_awaitable = 0b00000001,

	/**
	 * @brief Promise is being awaited
	 */
	sf_awaited = 0b00000010,

	/**
	 * @brief Promise has a result
	 */
	sf_ready = 0b00000100,

	/**
	 * @brief Promise has completed, no more results are expected
	 */
	sf_done = 0b00001000,

	/**
	 * @brief Promise was broken - future or promise is gone
	 */
	sf_broken = 0b0010000
};

template <typename T>
class promise_base;

/**
 * @brief Empty result from void-returning awaitable
 */
struct empty{};

/**
 * @brief Variant for the 3 conceptual values of a coroutine:
 */
template <typename T>
using result_t = std::variant<std::monostate, std::conditional_t<std::is_void_v<T>, empty, T>, std::exception_ptr>;

template <typename T>
void spawn_sync_wait_job(auto* awaitable, std::condition_variable &cv, auto&& result);

} /* namespace detail::promise */

template <typename Derived>
class basic_awaitable {
protected:
	/**
	 * @brief Implementation for sync_wait. This is code used by sync_wait, sync_wait_for, sync_wait_until.
	 *
	 * @tparam Timed Whether the wait function times out or not
	 * @param do_wait Function to do the actual wait on the cv
	 * @return If T is void, returns a boolean for which true means the awaitable completed, false means it timed out.
	 * @return If T is non-void, returns a std::optional<T> for which an absence of value means timed out.
	 */
	template <bool Timed>
	auto sync_wait_impl(auto&& do_wait) {
		using result_type = decltype(detail::co_await_resolve(std::declval<Derived>()).await_resume());
		using variant_type = detail::promise::result_t<result_type>;
		variant_type result;
		std::condition_variable cv;

		detail::promise::spawn_sync_wait_job<result_type>(static_cast<Derived*>(this), cv, result);
		do_wait(cv, result);
		/*
		 * Note: we use .index() here to support dpp::promise<std::exception_ptr> & dpp::promise<std::monostate> :D
		 */
		if (result.index() == 2) {
			std::rethrow_exception(std::get<2>(result));
		}
		if constexpr (!Timed) { // no timeout
			if constexpr (!std::is_void_v<result_type>) {
				return std::get<1>(result);
			}
		} else { // timeout
			if constexpr (std::is_void_v<result_type>) {
				return result.index() == 1 ? true : false;
			} else {
				return result.index() == 1 ? std::optional<result_type>{std::get<1>(result)} : std::nullopt;
			}
		}
	}

public:
	/**
	 * @brief Blocks this thread and waits for the awaitable to finish.
	 *
	 * @attention This will BLOCK THE THREAD. It is likely you want to use co_await instead.
	 * @return If T is void, returns a boolean for which true means the awaitable completed, false means it timed out.
	 * @return If T is non-void, returns a std::optional<T> for which an absence of value means timed out.
	 */
	auto sync_wait() {
		return sync_wait_impl<false>([](std::condition_variable &cv, auto&& result) {
			std::mutex m{};
			std::unique_lock lock{m};
			cv.wait(lock, [&result] { return result.index() != 0; });
		});
	}

	/**
	 * @brief Blocks this thread and waits for the awaitable to finish.
	 *
	 * @attention This will BLOCK THE THREAD. It is likely you want to use co_await instead.
	 * @param duration Maximum duration to wait for
	 * @return If T is void, returns a boolean for which true means the awaitable completed, false means it timed out.
	 * @return If T is non-void, returns a std::optional<T> for which an absence of value means timed out.
	 */
	template <class Rep, class Period>
	auto sync_wait_for(const std::chrono::duration<Rep, Period>& duration) {
		return sync_wait_impl<true>([duration](std::condition_variable &cv, auto&& result) {
			std::mutex m{};
			std::unique_lock lock{m};
			cv.wait_for(lock, duration, [&result] { return result.index() != 0; });
		});
	}

	/**
	 * @brief Blocks this thread and waits for the awaitable to finish.
	 *
	 * @attention This will BLOCK THE THREAD. It is likely you want to use co_await instead.
	 * @param time Maximum time point to wait for
	 * @return If T is void, returns a boolean for which true means the awaitable completed, false means it timed out.
	 * @return If T is non-void, returns a std::optional<T> for which an absence of value means timed out.
	 */
	template <class Clock, class Duration>
	auto sync_wait_until(const std::chrono::time_point<Clock, Duration> &time) {
		return sync_wait_impl<true>([time](std::condition_variable &cv, auto&& result) {
			std::mutex m{};
			std::unique_lock lock{m};
			cv.wait_until(lock, time, [&result] { return result.index() != 0; });
		});
	}
};

/**
 * @brief Generic awaitable class, represents a future value that can be co_await-ed on.
 *
 * Roughly equivalent of std::future for coroutines, with the crucial distinction that the future does not own a reference to a "shared state".
 * It holds a non-owning reference to the promise, which must be kept alive for the entire lifetime of the awaitable.
 *
 * @tparam T Type of the asynchronous value
 * @see promise
 */
template <typename T>
class awaitable : public basic_awaitable<awaitable<T>> {
protected:
	friend class detail::promise::promise_base<T>;

	using shared_state = detail::promise::promise_base<T>;
	using state_flags = detail::promise::state_flags;
	
	/**
	 * @brief The type of the result produced by this task.
	 */
	using result_type = T;

	/**
	 * @brief Non-owning pointer to the promise, which must be kept alive for the entire lifetime of the awaitable.
	 */
	shared_state *state_ptr = nullptr;

	/**
	 * @brief Construct from a promise.
	 *
	 * @param promise The promise to refer to.
	 */
	awaitable(shared_state *promise) noexcept : state_ptr{promise} {}

	/**
	 * @brief Abandons the promise.
	 *
	 * Set the promise's state to broken and unlinks this awaitable.
	 *
	 * @return uint8_t Flags previously held before setting them to broken
	 */
	uint8_t abandon();
	/**
	 * @brief Awaiter returned by co_await.
	 *
	 * Contains the await_ready, await_suspend and await_resume functions required by the C++ standard.
	 * This class is CRTP-like, in that it will refer to an object derived from awaitable.
	 *
	 * @tparam Derived Type of reference to refer to the awaitable.
	 */
	template <typename Derived>
	struct awaiter {
		Derived awaitable_obj;

		/**
		 * @brief First function called by the standard library when co_await-ing this object.
		 *
		 * @throws dpp::logic_exception If the awaitable's valid() would return false.
		 * @return bool Whether the result is ready, in which case we don't need to suspend
		 */
		bool await_ready() const;

		/**
		 * @brief Second function called by the standard library when co_await-ing this object.
		 *
		 * @throws dpp::logic_exception If the awaitable's valid() would return false.
		 * At this point the coroutine frame was allocated and suspended.
		 *
		 * @return bool Whether we do need to suspend or not
		 */
		bool await_suspend(detail::std_coroutine::coroutine_handle<> handle);

		/**
		 * @brief Third and final function called by the standard library when co_await-ing this object, after resuming.
		 *
		 * @throw ? Any exception that occured during the retrieval of the value will be thrown
		 * @return T The result.
		 */
		T await_resume();
	};

public:
	/**
	 * @brief Construct an empty awaitable.
	 *
	 * Such an awaitable must be assigned a promise before it can be awaited.
	 */
	awaitable() = default;

	/**
	 * @brief Copy construction is disabled.
	 */
	awaitable(const awaitable&) = delete;

	/**
	 * @brief Move from another awaitable.
	 *
	 * @param rhs The awaitable to move from, left in an unspecified state after this.
	 */
	awaitable(awaitable&& rhs) noexcept : state_ptr(std::exchange(rhs.state_ptr, nullptr)) {
	}

	/**
	 * @brief Title :)
	 *
	 * We use this in the destructor
	 */
	void if_this_causes_an_invalid_read_your_promise_was_destroyed_before_your_awaitable____check_your_promise_lifetime() {
		abandon();
	}

	/**
	 * @brief Destructor.
	 *
	 * May signal to the promise that it was destroyed.
	 */
	~awaitable();

	/**
	 * @brief Copy assignment is disabled.
	 */
	awaitable& operator=(const awaitable&) = delete;

	/**
	 * @brief Move from another awaitable.
	 *
	 * @param rhs The awaitable to move from, left in an unspecified state after this.
	 * @return *this
	 */
	awaitable& operator=(awaitable&& rhs) noexcept {
		abandon();
		state_ptr = std::exchange(rhs.state_ptr, nullptr);
		return *this;
	}

	/**
	 * @brief Check whether this awaitable refers to a valid promise.
	 *
	 * @return bool Whether this awaitable refers to a valid promise or not
	 */
	bool valid() const noexcept;

	/**
	 * @brief Check whether or not co_await-ing this would suspend the caller, i.e. if we have the result or not
	 *
	 * @return bool Whether we already have the result or not
	 */
	bool await_ready() const;

	/**
	 * @brief Overload of the co_await operator.
	 *
	 * @return Returns an @ref awaiter referencing this awaitable.
	 */
	template <typename Derived>
	requires (std::is_base_of_v<awaitable, std::remove_cv_t<Derived>>)
	friend awaiter<Derived&> operator co_await(Derived& obj) noexcept {
		return {obj};
	}

	/**
	 * @brief Overload of the co_await operator. Returns an @ref awaiter referencing this awaitable.
	 *
	 * @return Returns an @ref awaiter referencing this awaitable.
	 */
	template <typename Derived>
	requires (std::is_base_of_v<awaitable, std::remove_cv_t<Derived>>)
	friend awaiter<Derived&&> operator co_await(Derived&& obj) noexcept {
		return {std::move(obj)};
	}
};

namespace detail::promise {

/**
 * @brief Base class defining logic common to all promise types, aka the "write" end of an awaitable.
 */
template <typename T>
class promise_base {
protected:
	friend class awaitable<T>;

	/**
	 * @brief Variant representing one of either 3 states of the result value : empty, result, exception.
	 */
	using storage_type = result_t<T>;

	/**
	 * @brief State of the result value.
	 *
	 * @see storage_type
	 *
	 * @note use .index() instead of std::holds_alternative to support promise_base<std::exception_ptr> and promise_base<std::monostate> :)
	 */
	storage_type value = std::monostate{};

	/**
	 * @brief State of the awaitable tied to this promise.
	 */
	std::atomic<uint8_t> state = sf_none;

	/**
	 * @brief Coroutine handle currently awaiting the completion of this promise.
	 */
	std_coroutine::coroutine_handle<> awaiter = nullptr;

	/**
	 * @brief Check if the result is empty, throws otherwise.
	 *
	 * @throw dpp::logic_exception if the result isn't empty.
	 */
	void throw_if_not_empty() {
		if (value.index() != 0) [[unlikely]] {
			throw dpp::logic_exception("cannot set a value on a promise that already has one");
		}
	}

	/**
	 * @brief Unlinks this promise from its currently linked awaiter and returns it.
	 *
	 * At the time of writing this is only used in the case of a serious internal error in dpp::task.
	 * Avoid using this as this will crash if the promise is used after this.
	 */
	std_coroutine::coroutine_handle<> release_awaiter() {
		return std::exchange(awaiter, nullptr);
	}

	/**
	 * @brief Construct a new promise, with empty result.
	 */
	promise_base() = default;

	/**
	 * @brief Copy construction is disabled.
	 */
	promise_base(const promise_base&) = delete;

	/**
	 * @brief Move construction is disabled.
	 *
	 * awaitable hold a pointer to this object so moving is not possible.
	 */
	promise_base(promise_base&& rhs) = delete;

public:
	/**
	 * @brief Copy assignment is disabled.
	 */
	promise_base &operator=(const promise_base&) = delete;

	/**
	 * @brief Move assignment is disabled.
	 */
	promise_base &operator=(promise_base&& rhs) = delete;

	/**
	 * @brief Set this promise to an exception and resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws dpp::logic_exception if the promise is not empty.
	 * @throws ? Any exception thrown by the coroutine if resumed will propagate
	 */
	template <bool Notify = true>
	void set_exception(std::exception_ptr ptr) {
		throw_if_not_empty();
		value.template emplace<2>(std::move(ptr));
		[[maybe_unused]] auto previous_value = this->state.fetch_or(sf_ready, std::memory_order::acq_rel);
		if constexpr (Notify) {
			if ((previous_value & sf_awaited) != 0) {
				this->awaiter.resume();
			}
		}
	}

	/**
	 * @brief Notify a currently awaiting coroutine that the result is ready.
	 *
	 * @note This may resume the coroutine on the current thread.
	 * @throws ? Any exception thrown by the coroutine if resumed will propagate
	 */
	void notify_awaiter() {
		if ((state.load(std::memory_order::acquire) & sf_awaited) != 0) {
			awaiter.resume();
		}
	}

	/**
	 * @brief Get an awaitable object for this promise.
	 *
	 * @throws dpp::logic_exception if get_awaitable has already been called on this object.
	 * @return awaitable<T> An object that can be co_await-ed to retrieve the value of this promise.
	 */
	awaitable<T> get_awaitable() {
		uint8_t previous_flags = state.fetch_or(sf_has_awaitable, std::memory_order::relaxed);
		if (previous_flags & sf_has_awaitable) [[unlikely]] {
			throw dpp::logic_exception{"an awaitable was already created from this promise"};
		}
		return {this};
	}
};

}

/**
 * @brief Generic promise class, represents the owning potion of an asynchronous value.
 *
 * This class is roughly equivalent to std::promise, with the crucial distinction that the promise *IS* the shared state.
 * As such, the promise needs to be kept alive for the entire time a value can be retrieved.
 *
 * @tparam T Type of the asynchronous value
 * @see awaitable
 */
template <typename T>
class basic_promise : public detail::promise::promise_base<T> {
public:
	using detail::promise::promise_base<T>::promise_base;
	using detail::promise::promise_base<T>::operator=;

	/**
	 * @brief Construct the result in place by forwarding the arguments, and by default resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws dpp::logic_exception if the promise is not empty.
	 */
	template <bool Notify = true, typename... Args>
	requires (std::constructible_from<T, Args...>)
	void emplace_value(Args&&... args) {
		this->throw_if_not_empty();
		try {
			this->value.template emplace<1>(std::forward<Args>(args)...);
		} catch (...) {
			this->value.template emplace<2>(std::current_exception());
		}
		[[maybe_unused]] auto previous_value = this->state.fetch_or(detail::promise::sf_ready, std::memory_order::acq_rel);
		if constexpr (Notify) {
			if (previous_value & detail::promise::sf_awaited) {
				this->awaiter.resume();
			}
		}
	}

	/**
	 * @brief Construct the result by forwarding reference, and resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws dpp::logic_exception if the promise is not empty.
	 */
	template <bool Notify = true, typename U = T>
	requires (std::convertible_to<U&&, T>)
	void set_value(U&& v) {
		emplace_value<Notify>(std::forward<U>(v));
	}

	/**
	 * @brief Construct a void result, and resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws dpp::logic_exception if the promise is not empty.
	 */
	template <bool Notify = true>
	requires (std::is_void_v<T>)
	void set_value() {
		this->throw_if_not_empty();
		this->value.template emplace<1>();
		[[maybe_unused]] auto previous_value = this->state.fetch_or(detail::promise::sf_ready, std::memory_order::acq_rel);
		if constexpr (Notify) {
			if (previous_value & detail::promise::sf_awaited) {
				this->awaiter.resume();
			}
		}
	}
};

/**
 * @brief Generic promise class, represents the owning potion of an asynchronous value.
 *
 * This class is roughly equivalent to std::promise, with the crucial distinction that the promise *IS* the shared state.
 * As such, the promise needs to be kept alive for the entire time a value can be retrieved.
 *
 * The difference between basic_promise and this object is that this one is moveable as it wraps an underlying basic_promise in a std::unique_ptr.
 *
 * @see awaitable
 */
template <typename T>
class moveable_promise {
	/**
	 * @brief Shared state, wrapped in a unique_ptr to allow move without disturbing an awaitable's promise pointer.
	 */
	std::unique_ptr<basic_promise<T>> shared_state = std::make_unique<basic_promise<T>>();

public:
	/**
	 * @copydoc basic_promise<T>::emplace_value
	 */
	template <bool Notify = true, typename... Args>
	requires (std::constructible_from<T, Args...>)
	void emplace_value(Args&&... args) {
		shared_state->template emplace_value<Notify>(std::forward<Args>(args)...);
	}

	/**
	 * @copydoc basic_promise<T>::set_value(U&&)
	 */
	template <bool Notify = true, typename U = T>
	void set_value(U&& v) requires (std::convertible_to<U&&, T>) {
		shared_state->template set_value<Notify>(std::forward<U>(v));
	}

	/**
	 * @copydoc basic_promise<T>::set_value()
	 */
	template <bool Notify = true>
	void set_value() requires (std::is_void_v<T>) {
		shared_state->template set_value<Notify>();
	}

	/**
	 * @copydoc basic_promise<T>::set_value(T&&)
	 */
	template <bool Notify = true>
	void set_exception(std::exception_ptr ptr) {
		shared_state->template set_exception<Notify>(std::move(ptr));
	}

	/**
	 * @copydoc basic_promise<T>::notify_awaiter
	 */
	void notify_awaiter() {
		shared_state->notify_awaiter();
	}

	/**
	 * @copydoc basic_promise<T>::get_awaitable
	 */
	awaitable<T> get_awaitable() {
		return shared_state->get_awaitable();
	}
};

template <typename T>
using promise = moveable_promise<T>;

template <typename T>
auto awaitable<T>::abandon() -> uint8_t {
	uint8_t previous_state = state_flags::sf_broken;
	if (state_ptr) {
		previous_state = state_ptr->state.fetch_or(state_flags::sf_broken, std::memory_order::acq_rel);
		state_ptr = nullptr;
	}
	return previous_state;
}

template <typename T>
awaitable<T>::~awaitable() {
	if_this_causes_an_invalid_read_your_promise_was_destroyed_before_your_awaitable____check_your_promise_lifetime();
}

template <typename T>
bool awaitable<T>::valid() const noexcept {
	return state_ptr != nullptr;
}

template <typename T>
bool awaitable<T>::await_ready() const {
	if (!this->valid()) {
		throw dpp::logic_exception("cannot co_await an empty awaitable");
	}
	uint8_t state = this->state_ptr->state.load(std::memory_order::relaxed);
	return state & detail::promise::sf_ready;
}

template <typename T>
template <typename Derived>
bool awaitable<T>::awaiter<Derived>::await_suspend(detail::std_coroutine::coroutine_handle<> handle) {
	auto &promise = *awaitable_obj.state_ptr;

	promise.awaiter = handle;
	auto previous_flags = promise.state.fetch_or(detail::promise::sf_awaited, std::memory_order::relaxed);
	if (previous_flags & detail::promise::sf_awaited) {
		throw dpp::logic_exception("awaitable is already being awaited");
	}
	return !(previous_flags & detail::promise::sf_ready);
}

template <typename T>
template <typename Derived>
T awaitable<T>::awaiter<Derived>::await_resume() {
	auto &promise = *awaitable_obj.state_ptr;

	promise.state.fetch_and(~detail::promise::sf_awaited, std::memory_order::acq_rel);
	if (std::holds_alternative<std::exception_ptr>(promise.value)) {
		std::rethrow_exception(std::get<2>(promise.value));
	}
	if constexpr (!std::is_void_v<T>) {
		return std::get<1>(std::move(promise.value));
	} else {
		return;
	}
}



template <typename T>
template <typename Derived>
bool awaitable<T>::awaiter<Derived>::await_ready() const {
	return static_cast<Derived>(awaitable_obj).await_ready();
}

}

#include <dpp/coro/job.h>

namespace dpp {

namespace detail::promise {

template <typename T>
void spawn_sync_wait_job(auto* awaitable, std::condition_variable &cv, auto&& result) {
	[](auto* awaitable_, std::condition_variable &cv_, auto&& result_) -> dpp::job {
		try {
			if constexpr (std::is_void_v<T>) {
				co_await *awaitable_;
				result_.template emplace<1>();
			} else {
				result_.template emplace<1>(co_await *awaitable_);
			}
		} catch (...) {
			result_.template emplace<2>(std::current_exception());
		}
		cv_.notify_all();
	}(awaitable, cv, std::forward<decltype(result)>(result));
}

}

}

#endif /* DPP_CORO */
