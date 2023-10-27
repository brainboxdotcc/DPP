\page coro-introduction Introduction to coroutines

Introduced in C++20, coroutines are the solution to the impracticality of callbacks. In short, a coroutine is a function that can be paused and resumed later. They are an extremely powerful alternative to callbacks for asynchronous APIs in particular, as the function can be paused when waiting for an API response, and resumed when it is received.

Let's revisit \ref attach-file "attaching a downloaded file", but this time with a coroutine:


\include{cpp} coro_intro.cpp

Coroutines can make commands simpler by eliminating callbacks, which can be very handy in the case of complex commands that rely on a lot of different data or steps.

In order to be a coroutine, a function has to return a special type with special functions; D++ offers dpp::job, dpp::task, and dpp::coroutine, which are designed to work seamlessly with asynchronous calls through dpp::async, which all the functions starting with `co_` such as dpp::cluster::co_message_create return. Event routers can have a dpp::task coroutine attached to them, as this object allows to create coroutines that can execute on their own, asynchronously. More on that and the difference between it and the other two types later. To turn a function into a coroutine, simply make it return dpp::task<void> as seen in the example at line 10, then use `co_await` on awaitable types or `co_return`. The moment the execution encounters one of these two keywords, the function is transformed into a coroutine. Coroutines that use dpp::task<void> can be used for event handlers, they can be attached to an event router just the same way as regular event handlers.

When using a `co_*` function such as `co_message_create`, the request is sent immediately and the returned dpp::async can be `co_await`-ed, at which point the coroutine suspends (pauses) and returns back to its caller; in other words, the program is free to go and do other things while the data is being retrieved and D++ will resume your coroutine when it has the data you need, which will be returned from the `co_await` expression.

\warning As a rule of thumb when making coroutines, **always prefer taking parameters by value and avoid lambda capture**! See below for an explanation.

You may hear that coroutines are "writing async code as if it was sync", while this is sort of correct, it may limit your understanding and especially the dangers of coroutines. I find **they are best thought of as a shortcut for a state machine**, if you've ever written one, you know what this means. Think of the lambda as *its constructor*, in which captures are variable parameters. Think of the parameters passed to your lambda as data members in your state machine. When you `co_await` something, the state machine's function exits, the program goes back to the caller, at this point the calling function may return. References are kept as references in the state machine, which means by the time the state machine is resumed, the reference may be dangling: \ref lambdas-and-locals "this is not good"!

Another way to think of them is just like callbacks but keeping the current scope intact. In fact this is exactly what it is, the co_* functions call the normal API calls, with a callback that resumes the coroutine, *in the callback thread*. This means you cannot rely on thread_local variables and need to keep in mind concurrency issues with global states, as your coroutine will be resumed in another thread than the one it started on.
