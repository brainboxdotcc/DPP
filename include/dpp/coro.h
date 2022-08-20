#ifdef DPP_CORO
#pragma once
#include <coroutine>
#include <dpp/restresults.h>

namespace dpp {
	
	/**
	 * @brief 
	 */
	using handle_type = std::coroutine_handle<struct promise>;

	class cluster;

	/**
	 * @brief 
	 */
	struct task {
		/**
		 * @brief 
		 */
		using promise_type = dpp::promise;
	};

	/**
	 * @brief 
	 */
	struct promise {
		/**
		 * @brief 
		 */
		cluster* bot = nullptr;

		/**
		 * @brief 
		 */
		confirmation_callback_t callback;

		/**
		 * @brief Construct a new promise object
		 */
		promise() = default;

		/**
		 * @brief Construct a new promise object
		 * 
		 * @param ev 
		 */
		promise(const dpp::event_dispatch_t& ev) : bot(ev.from->creator) { }

		/**
		 * @brief Get the return object object
		 * 
		 * @return auto 
		 */
		auto get_return_object() {
			return task{};
		}

		/**
		 * @brief 
		 * 
		 * @return auto 
		 */
		auto initial_suspend() noexcept {
			return std::suspend_never{};
		}

		/**
		 * @brief 
		 * 
		 * @return auto 
		 */
		auto final_suspend() noexcept {
			return std::suspend_never{};
		}

		/**
		 * @brief 
		 */
		void return_void() noexcept {}

		/**
		 * @brief 
		 */
		void unhandled_exception() { 
			/* try { std::rethrow_exception(std::current_exception()); } */ 
			/* catch (const std::exception& e) { std::cout << e.what() << '\n'; } */ 
		}
	};

	/**
	 * @brief 
	 * 
	 * @tparam T 
	 */
	template <typename T> 
	struct awaitable {
		/**
		 * @brief 
		 */
		promise* p;
		
		/**
		 * @brief 
		 * 
		 */
		cluster* bot;

		/**
		 * @brief 
		 */
		T api_req;

		/**
		 * @brief Construct a new awaitable object
		 * 
		 * @param cl 
		 * @param api_call 
		 */
		awaitable(cluster* cl, T api_call) : bot{cl}, api_req{api_call} {} 

		/**
		 * @brief 
		 * 
		 * @return auto 
		 */
		auto await_ready() noexcept {
			return false;
		}

		/**
		 * @brief 
		 * 
		 * @param handle 
		 * @return auto 
		 */
		auto await_suspend(handle_type handle) { 
			p = &handle.promise();
			if (!p->bot) p->bot = bot;
			api_req([handle](const confirmation_callback_t& cback) { handle.promise().callback = cback; handle.resume(); });
		}

		/**
		 * @brief 
		 * 
		 * @return auto 
		 */
		auto await_resume() {
			return p->callback;
		}
	};

};

/* template<> */
/* struct std::coroutine_traits<void, const dpp::interaction_create_t&> { */
/* 	using promise_type = dpp::promise; */
/* }; */
#endif
