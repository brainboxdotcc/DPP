#ifdef DPP_CORO
#pragma once

#if !(defined( _MSC_VER ) || defined( _CONSOLE ) || defined( __GNUC__ )) // if libc++
#  define EXPERIMENTAL_COROUTINE
#endif

#ifdef EXPERIMENTAL_COROUTINE
#  include <experimental/coroutine>
#else
#  include <coroutine>
#endif

#include <memory>
#include <utility>
#include <type_traits>

namespace dpp {
	class cluster;
	struct confirmation_callback_t;

	/**
	 * @brief Implementation details for internal use only.
	 *
	 * @attention This is only meant to be used by D++ internally. Support will not be given regarding the facilities in this namespace.
	 */
	namespace detail {
#ifdef _DOXYGEN_
		/**
		 * @brief Alias for either std or std::experimental depending on compiler and library. Used by coroutine implementation.
		 *
		 * @todo Remove and use std when all supported libraries have coroutines in it
		 */
		namespace std_coroutine {}
#else
#  ifdef EXPERIMENTAL_COROUTINE
		namespace std_coroutine = std::experimental;
#  else
		namespace std_coroutine = std;
#  endif
#endif

		/**
		* @brief A task's promise type, with special logic for handling nested tasks.
		*/
		template <typename ReturnType>
		struct task_promise;

		/**
		 * @brief The object automatically co_await-ed at the end of a task. Ensures nested task chains are resolved, and the promise cleans up if it needs to.
		 */
		template <typename ReturnType>
		struct task_chain_final_awaiter;

		/**
		* @brief Alias for <a href="https://en.cppreference.com/w/cpp/coroutine/coroutine_handle">std::coroutine_handle</a> for a task_promise.
		*/
		template <typename ReturnType>
		using task_handle = detail::std_coroutine::coroutine_handle<detail::task_promise<ReturnType>>;
	}

	/**
	 * @brief A coroutine task. It can be co_awaited to make nested coroutines.
	 *
	 * Can be used in conjunction with coroutine events via dpp::event_router_t::co_attach, or on its own.
	 *
	 * @warning This feature is EXPERIMENTAL. The API may change at any time and there may be bugs. Please report any to <a href="https://github.com/brainboxdotcc/DPP/issues">GitHub issues</a> or to the <a href="https://discord.gg/dpp">D++ Discord server</a>.
	 * @tparam ReturnType Return type of the coroutine. Can be void, or a complete object that supports move construction and move assignment.
	 */
	template <typename ReturnType>
#ifndef _DOXYGEN_
	requires std::is_same_v<void, ReturnType> || (!std::is_reference_v<ReturnType> && std::is_move_constructible_v<ReturnType> && std::is_move_assignable_v<ReturnType>)
#endif
	class task {
		/**
		 * @brief The coroutine handle of this task.
		 */
		detail::task_handle<ReturnType> handle;

		/**
		 * @brief Promise type of this coroutine. For internal use only, do not use.
		 */
		friend struct detail::task_promise<ReturnType>;

		/**
		 * @brief Construct from a coroutine handle. Internal use only
		 */
		explicit task(detail::task_handle<ReturnType> handle_) : handle(handle_) {}

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
			return handle.done();
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
		bool await_suspend(detail::task_handle<T> caller) noexcept {
			auto &my_promise = handle.promise();

			if (my_promise.is_sync)
				return false;
			my_promise.parent = caller;
			caller.promise().is_sync = false;	
			return true;
		}

		/**
		 * @brief Function called by the standard library when the coroutine is resumed.
		 *
		 * @remark Do not call this manually, use the co_await keyword instead.
		 * @throw Throws any exception thrown or uncaught by the coroutine
		 * @return ReturnType The result of the coroutine. It is the value the whole co-await expression evaluates to
		 */
		ReturnType await_resume();

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
		template <typename ReturnType>
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
			void await_suspend(detail::task_handle<ReturnType> handle) noexcept;

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
			std::suspend_never initial_suspend() noexcept {
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
		template <typename ReturnType>
		struct task_promise : task_promise_base {
			/**
			 * @brief Stored return value of the coroutine.
			 *
			 * @details The main reason we use std::optional<ReturnType> here and not ReturnType is to avoid default construction of the value so we only require ReturnType to have a move constructor, instead of both a default constructor and move assignment operator
			 */
			std::optional<ReturnType> value = std::nullopt;

			/**
			 * @brief Function called by the standard library when the coroutine co_returns a value.
			 *
			 * Stores the value internally to hand to the caller when it resumes.
			 *
			 * @param expr The value given to co_return
			 */
			void return_value(ReturnType expr) {
				value = std::move(expr);
			}

			/**
			 * @brief Function called by the standard library when the coroutine is created.
			 *
			 * @return task The coroutine object
			 */
			task<ReturnType> get_return_object() {
				return task{task_handle<ReturnType>::from_promise(*this)};
			}

			/**
			 * @brief Function called by the standard library when the coroutine reaches its last suspension point
			 *
			 * @return task_chain_final_awaiter Special object containing the chain resolution and clean-up logic.
			 */
			task_chain_final_awaiter<ReturnType> final_suspend() noexcept {
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

		template <typename ReturnType>
		void detail::task_chain_final_awaiter<ReturnType>::await_suspend(detail::task_handle<ReturnType> handle) noexcept {
			task_promise<ReturnType> &promise = handle.promise();
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
	}

	template <typename ReturnType>
#ifndef _DOXYGEN_
	requires std::is_same_v<void, ReturnType> || (!std::is_reference_v<ReturnType> && std::is_move_constructible_v<ReturnType> && std::is_move_assignable_v<ReturnType>)
#endif
	ReturnType task<ReturnType>::await_resume() {
		if (handle.promise().exception) // If we have an exception, rethrow
			std::rethrow_exception(handle.promise().exception);
		if constexpr (!std::is_same_v<ReturnType, void>) // If we have a return type, return it and clean up our stored value
			return std::forward<ReturnType>(*std::exchange(handle.promise().value, std::nullopt));
	}

	/**
	 * @brief A co_await-able object handling an API call.
	 *
	 * This class is the return type of the dpp::cluster::co_* methods, but it can also be created manually to wrap any async call.
	 *
	 * @remark - This object's methods, other than constructors and operators, should not be called directly. It is designed to be used with coroutine keywords such as co_await.
	 * @remark - This object must not be co_await-ed more than once.
	 * @remark - The coroutine may be resumed in another thread, do not rely on thread_local variables.
	 * @warning This feature is EXPERIMENTAL. The API may change at any time and there may be bugs. Please report any to <a href="https://github.com/brainboxdotcc/DPP/issues">GitHub issues</a> or to the <a href="https://discord.gg/dpp">D++ Discord server</a>.
	 * @tparam ReturnType The return type of the API call. Defaults to confirmation_callback_t
	 */
	template <typename ReturnType = confirmation_callback_t>
	class awaitable {
		/**
		 * @brief Ref-counted callback, contains the callback logic and manages the lifetime of the callback data over multiple threads.
		 */
		struct shared_callback {
			/**
			 * @brief State of the awaitable and its callback.
			 */
			struct callback_state {
				enum state_t {
					waiting,
					done,
					dangling
				};

				/**
				 * @brief Mutex to ensure the API result isn't set at the same time the coroutine is awaited and its value is checked, or the awaitable is destroyed
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
				std::optional<ReturnType> result = std::nullopt;

				/**
				 * @brief Handle to the coroutine co_await-ing on this API call
				 *
				 * @see <a href="https://en.cppreference.com/w/cpp/coroutine/coroutine_handle">std::coroutine_handle</a>
				 */
				std::coroutine_handle<> coro_handle = nullptr;
			};

			/**
			 * @brief Callback function.
			 *
			 * @param cback The result of the API call.
			 */
			void operator()(const ReturnType &cback) const {
				std::unique_lock lock{get_mutex()};

				if (state->state == callback_state::dangling) // Awaitable is gone - likely an exception killed it or it was never co_await-ed
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
			 * @brief Function called by the awaitable when it is destroyed when it was never co_awaited, signals to the callback to abort.
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
			std::optional<ReturnType> &get_result() const {
				return (state->result);
			}

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

			callback_state *state;
		};

		/**
		 * @brief Shared state of the awaitable and its callback, to be used across threads.
		 */
		shared_callback api_callback;

	public:
		/**
		 * @brief Construct an awaitable wrapping an object method, the call is made immediately by forwarding to <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a> and can be awaited later to retrieve the result.
		 *
		 * @param obj The object to call the method on
		 * @param fun The method of the object to call. Its last parameter must be a callback taking a parameter of type ReturnType
		 * @param args Parameters to pass to the method, excluding the callback
		 */
		template <typename Obj, typename Fun, typename... Args>
#ifndef _DOXYGEN_
		requires std::invocable<Fun, Obj, Args..., std::function<void(ReturnType)>>
#endif
		awaitable(Obj &&obj, Fun &&fun, Args&&... args) : api_callback{} {
			std::invoke(std::forward<Fun>(fun), std::forward<Obj>(obj), std::forward<Args>(args)..., api_callback);
		}

		/**
		 * @brief Construct an awaitable wrapping an invokeable object, the call is made immediately by forwarding to <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a> and can be awaited later to retrieve the result.
		 *
		 * @param fun The object to call using <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a>. Its last parameter must be a callable taking a parameter of type ReturnType
		 * @param args Parameters to pass to the object, excluding the callback
		 */
		template <typename Fun, typename... Args>
#ifndef _DOXYGEN_
		requires std::invocable<Fun, Args..., std::function<void(ReturnType)>>
#endif
		awaitable(Fun &&fun, Args&&... args) : api_callback{} {
			std::invoke(std::forward<Fun>(fun), std::forward<Args>(args)..., api_callback);
		}

		/**
		 * @brief Destructor. If any callback is pending it will be aborted.
		 *
		 */
		~awaitable() {
			api_callback.set_dangling();
		}

		/**
		 * @brief Copy constructor is disabled
		 */
		awaitable(const awaitable &) = delete;

		/**
		 * @brief Move constructor
		 *
		 * NOTE: Despite being marked noexcept, this function uses std::lock_guard which may throw. The implementation assumes this can never happen, hence noexcept. Report it if it does, as that would be a bug.
		 *
		 * @remark Using the moved-from awaitable after this function is undefined behavior.
		 * @param other The awaitable object to move the data from.
		 */
		awaitable(awaitable &&other) noexcept = default;

		/**
		 * @brief Copy assignment is disabled
		 */
		awaitable &operator=(const awaitable &) = delete;

		/**
		 * @brief Move assignment operator.
		 *
		 * NOTE: Despite being marked noexcept, this function uses std::lock_guard which may throw. The implementation assumes this can never happen, hence noexcept. Report it if it does, as that would be a bug.
		 *
		 * @remark Using the moved-from awaitable after this function is undefined behavior.
		 * @param other The awaitable object to move the data from
		 */
		awaitable &operator=(awaitable &&other) noexcept = default;

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
		bool await_suspend(detail::task_handle<T> handle) {
			std::lock_guard lock{api_callback.get_mutex()};

			if (api_callback.get_result().has_value())
				return false; // immediately resume the coroutine as we already have the result of the api call
			handle.promise().is_sync = false;
			api_callback.state->coro_handle = handle;
			return true; // suspend the caller, the callback will resume it
		}

		/**
		 * @brief Function called by the standard library when the awaitable is resumed. Its return value is what the whole co_await expression evaluates to
		 *
		 * @remark Do not call this manually, use the co_await keyword instead.
		 * @return ReturnType The result of the API call.
		 */
		ReturnType await_resume() {
			return std::move(*api_callback.get_result());
		}
	};
};

/**
 * @brief Specialization of std::coroutine_traits, helps the standard library figure out a promise type from a coroutine function.
 */
template<typename T, typename... Args>
struct dpp::detail::std_coroutine::coroutine_traits<dpp::task<T>, Args...> {
	using promise_type = dpp::detail::task_promise<T>;
};

#endif
