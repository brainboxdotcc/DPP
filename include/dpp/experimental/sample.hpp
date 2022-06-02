#include <experimental/coroutine>

namespace co = std::experimental;

struct awaitable {

};

struct promise_type {
	get_return_object();
	void return_void();
};
