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

namespace dpp {

struct coroutine_dummy {
	int *handle_dummy = nullptr;
};

}

#ifdef DPP_CORO

#include <dpp/coro/coro.h>
#include <dpp/coro/awaitable.h>

#include <optional>
#include <type_traits>
#include <exception>
#include <utility>
#include <type_traits>

namespace dpp {

namespace detail {

namespace coroutine {

template <typename R>
struct promise_t;

template <typename R>
/**
 * @brief Alias for the handle_t of a coroutine.
 */
using handle_t = std_coroutine::coroutine_handle<promise_t<R>>;

} // namespace coroutine

} // namespace detail

/**
 * @class coroutine coroutine.h coro/coroutine.h
 * @brief Base type for a coroutine, starts on co_await.
 *
 * @warning - This feature is EXPERIMENTAL. The API may change at any time and there may be bugs.
 * Please report any to <a href="https://github.com/brainboxdotcc/DPP/issues">GitHub Issues</a> or to our <a href="https://discord.gg/dpp">Discord Server</a>.
 * @warning - Using co_await on this object more than once is undefined behavior.
 * @tparam R Return type of the coroutine. Can be void, or a complete object that supports move construction and move assignment.
 */
template <typename R>
class [[nodiscard("dpp::coroutine only starts when it is awaited, it will do nothing if discarded")]] coroutine : public basic_awaitable<coroutine<R>> {
	/**
	 * @brief Promise has friend access for the constructor
	 */
	friend struct detail::coroutine::promise_t<R>;

	/**
	 * @brief Coroutine handle.
	 */
	detail::coroutine::handle_t<R> handle{nullptr};

	/**
	 * @brief Construct from a handle. Internal use only.
	 */
	coroutine(detail::coroutine::handle_t<R> h) : handle{h} {}

	struct awaiter {
		/**
		 * @brief Reference to the coroutine object being awaited.
		 */
		coroutine &coro;

		/**
		 * @brief First function called by the standard library when the coroutine is co_await-ed.
		 *
		 * @remark Do not call this manually, use the co_await keyword instead.
		 * @throws invalid_operation_exception if the coroutine is empty or finished.
		 * @return bool Whether the coroutine is done
		 */
		[[nodiscard]] bool await_ready() const {
			if (!coro.handle) {
				throw dpp::logic_exception("cannot co_await an empty coroutine");
			}
			return coro.handle.done();
		}

		/**
		 * @brief Second function called by the standard library when the coroutine is co_await-ed.
		 *
		 * Stores the calling coroutine in the promise to resume when this coroutine suspends.
		 *
		 * @remark Do not call this manually, use the co_await keyword instead.
		 * @param caller The calling coroutine, now suspended
		 */
		template <typename T>
		[[nodiscard]] detail::coroutine::handle_t<R> await_suspend(detail::std_coroutine::coroutine_handle<T> caller) noexcept {
			coro.handle.promise().parent = caller;
			return coro.handle;
		}

		/**
		 * @brief Final function called by the standard library when the coroutine is co_await-ed.
		 *
		 * Pops the coroutine's result and returns it.
		 * @remark Do not call this manually, use the co_await keyword instead.
		 */
		R await_resume() {
			detail::coroutine::promise_t<R> &promise = coro.handle.promise();
			if (promise.exception) {
				std::rethrow_exception(promise.exception);
			}
			if constexpr (!std::is_void_v<R>) {
				return *std::exchange(promise.result, std::nullopt);
			} else {
				return; // unnecessary but makes lsp happy
			}
		}
	};

public:
	/**
	 * @brief The type of the result produced by this coroutine.
	 */
	using result_type = R;

	/**
	 * @brief Default constructor, creates an empty coroutine.
	 */
	coroutine() = default;

	/**
	 * @brief Copy constructor is disabled
	 */
	coroutine(const coroutine &) = delete;

	/**
	 * @brief Move constructor, grabs another coroutine's handle
	 *
	 * @param other Coroutine to move the handle from
	 */
	coroutine(coroutine &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

	/**
	 * @brief Destructor, destroys the handle.
	 */
	~coroutine() {
		if (handle) {
			handle.destroy();
		}
	}

	/**
	 * @brief Copy assignment is disabled
	 */
	coroutine &operator=(const coroutine &) = delete;

	/**
	 * @brief Move assignment, grabs another coroutine's handle
	 *
	 * @param other Coroutine to move the handle from
	 */
	coroutine &operator=(coroutine &&other) noexcept {
		handle = std::exchange(other.handle, nullptr);
		return *this;
	}
	
	[[nodiscard]] auto operator co_await() {
		return awaiter{*this};
	}
};

namespace detail::coroutine {
	template <typename R>
	struct final_awaiter;

#ifdef DPP_CORO_TEST
	struct promise_t_base{};
#endif

	/**
	 * @brief Promise type for coroutine.
	 */
	template <typename R>
	struct promise_t {
		/**
		 * @brief Handle of the coroutine co_await-ing this coroutine.
		 */
		std_coroutine::coroutine_handle<> parent{nullptr};

		/**
		 * @brief Return value of the coroutine
		 */
		std::optional<R> result{};

		/**
		 * @brief Pointer to an uncaught exception thrown by the coroutine
		 */
		std::exception_ptr exception{nullptr};

#ifdef DPP_CORO_TEST
		promise_t() {
			++coro_alloc_count<promise_t_base>;
		}

		~promise_t() {
			--coro_alloc_count<promise_t_base>;
		}
#endif

		/**
		 * @brief Function called by the standard library when reaching the end of a coroutine
		 *
		 * @return final_awaiter<R> Resumes any coroutine co_await-ing on this
		 */
		[[nodiscard]] final_awaiter<R> final_suspend() const noexcept;

		/**
		 * @brief Function called by the standard library when the coroutine start
		 *
		 * @return @return <a href="https://en.cppreference.com/w/cpp/coroutine/suspend_always">std::suspend_always</a> Always suspend at the start, for a lazy start
		 */
		[[nodiscard]] std_coroutine::suspend_always initial_suspend() const noexcept {
			return {};
		}

		/**
		 * @brief Function called when an exception escapes the coroutine
		 *
		 * Stores the exception to throw to the co_await-er
		 */
		void unhandled_exception() noexcept {
			exception = std::current_exception();
		}

		/**
		 * @brief Function called by the standard library when the coroutine co_returns a value.
		 *
		 * Stores the value internally to hand to the caller when it resumes.
		 *
		 * @param expr The value given to co_return
		 */
		void return_value(R&& expr) noexcept(std::is_nothrow_move_constructible_v<R>) requires std::move_constructible<R> {
			result = static_cast<R&&>(expr);
		}

		/**
		 * @brief Function called by the standard library when the coroutine co_returns a value.
		 *
		 * Stores the value internally to hand to the caller when it resumes.
		 *
		 * @param expr The value given to co_return
		 */
		void return_value(const R &expr) noexcept(std::is_nothrow_copy_constructible_v<R>) requires std::copy_constructible<R> {
			result = expr;
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
			result = std::forward<T>(expr);
		}

		/**
		 * @brief Function called to get the coroutine object
		 */
		dpp::coroutine<R> get_return_object() {
			return dpp::coroutine<R>{handle_t<R>::from_promise(*this)};
		}
	};

	/**
	 * @brief Struct returned by a coroutine's final_suspend, resumes the continuation
	 */
	template <typename R>
	struct final_awaiter {
		/**
		 * @brief First function called by the standard library when reaching the end of a coroutine
		 *
		 * @return false Always return false, we need to suspend to resume the parent
		 */
		[[nodiscard]] bool await_ready() const noexcept {
			return false;
		}

		/**
		 * @brief Second function called by the standard library when reaching the end of a coroutine.
		 *
		 * @return std::handle_t<> Coroutine handle to resume, this is either the parent if present or std::noop_coroutine()
		 */
		[[nodiscard]] std_coroutine::coroutine_handle<> await_suspend(std_coroutine::coroutine_handle<promise_t<R>> handle) const noexcept {
			auto parent = handle.promise().parent;

			return parent ? parent : std_coroutine::noop_coroutine();
		}

		/**
		 * @brief Function called by the standard library when this object is resumed
		 */
		void await_resume() const noexcept {}
	};

	template <typename R>
	final_awaiter<R> promise_t<R>::final_suspend() const noexcept {
		return {};
	}

	/**
	 * @brief Struct returned by a coroutine's final_suspend, resumes the continuation
	 */
	template <>
	struct promise_t<void> {
		/**
		 * @brief Handle of the coroutine co_await-ing this coroutine.
		 */
		std_coroutine::coroutine_handle<> parent{nullptr};

		/**
		 * @brief Pointer to an uncaught exception thrown by the coroutine
		 */
		std::exception_ptr exception{nullptr};

		/**
		 * @brief Function called by the standard library when reaching the end of a coroutine
		 *
		 * @return final_awaiter<R> Resumes any coroutine co_await-ing on this
		 */
		[[nodiscard]] final_awaiter<void> final_suspend() const noexcept {
			return {};
		}

		/**
		 * @brief Function called by the standard library when the coroutine start
		 *
		 * @return @return <a href="https://en.cppreference.com/w/cpp/coroutine/suspend_always">std::suspend_always</a> Always suspend at the start, for a lazy start
		 */
		[[nodiscard]] std_coroutine::suspend_always initial_suspend() const noexcept {
			return {};
		}

		/**
		 * @brief Function called when an exception escapes the coroutine
		 *
		 * Stores the exception to throw to the co_await-er
		 */
		void unhandled_exception() noexcept {
			exception = std::current_exception();
		}

		/**
		 * @brief Function called when co_return is used
		 */
		void return_void() const noexcept {}

		/**
		 * @brief Function called to get the coroutine object
		 */
		[[nodiscard]] dpp::coroutine<void> get_return_object() {
			return dpp::coroutine<void>{handle_t<void>::from_promise(*this)};
		}
	};

} // namespace detail

DPP_CHECK_ABI_COMPAT(coroutine<void>, coroutine_dummy)
DPP_CHECK_ABI_COMPAT(coroutine<uint64_t>, coroutine_dummy)

} // namespace dpp

/**
 * @brief Specialization of std::coroutine_traits, helps the standard library figure out a promise type from a coroutine function.
 */
template<typename R, typename... Args>
struct dpp::detail::std_coroutine::coroutine_traits<dpp::coroutine<R>, Args...> {
	using promise_type = dpp::detail::coroutine::promise_t<R>;
};

#endif /* DPP_CORO */
