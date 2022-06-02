#ifdef __clang__
#include <experimental/coroutine>
namespace co = std::experimental;
#else 
#include <coroutine>
namespace co = std;
#endif

namespace dpp {
}

struct promise_type {
	auto initial_suspend() noexcept {}
	auto final_suspend() noexcept { return co::suspend_never{}; }
	auto return_void() {}
	auto unhandled_exceptions() noexcept {}
};

template <typename T>
struct rest_awaitable {
	T value;

	auto await_ready() {}
	auto await_suspend(co::coroutine_handle<promise_type>) {}
	auto await_resume() {}
};

