#ifdef DPP_CORO
#pragma once
#include <coroutine>
#include <dpp/restresults.h>

namespace dpp {
	using handle_type = std::coroutine_handle<struct promise>;

	class cluster;

	struct task {
		using promise_type = dpp::promise;
	};

	struct promise {
		cluster* bot = nullptr;
		confirmation_callback_t callback;

		promise() = default;
		promise(const dpp::event_dispatch_t& ev) : bot(ev.from->creator) { }
		auto get_return_object() { return task{}; }  // { return coroutine::from_promise(*this); }
		auto initial_suspend() noexcept { return std::suspend_never{}; }
		auto final_suspend() noexcept { return std::suspend_never{}; }
		void return_void() noexcept {}
		void unhandled_exception() { 
			/* try { std::rethrow_exception(std::current_exception()); } */ 
			/* catch (const std::exception& e) { std::cout << e.what() << '\n'; } */ 
		}
	};

	template <typename T> 
	struct awaitable {
		promise* p;
		cluster* bot;
		T api_req;
		awaitable(cluster* cl, T api_call) : bot{cl}, api_req{api_call} {} 
		auto await_ready() noexcept { return false; }
		auto await_suspend(handle_type handle) { 
			p = &handle.promise();
			if (!p->bot) p->bot = bot;
			api_req([handle](const confirmation_callback_t& cback) { handle.promise().callback = cback; handle.resume(); });
		}
		auto await_resume() { return p->callback; }
	};

};

/* template<> */
/* struct std::coroutine_traits<void, const dpp::interaction_create_t&> { */
/* 	using promise_type = dpp::promise; */
/* }; */
#endif
