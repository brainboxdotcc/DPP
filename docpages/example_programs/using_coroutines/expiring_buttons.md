\page expiring-buttons Making expiring buttons with when_any

\include{doc} coro_warn.dox

In the last example we've explored how to \ref awaiting-events "await events" using coroutines, we ran into the problem of the coroutine waiting forever if the button was never clicked. Wouldn't it be nice if we could add an "or" to our algorithm, for example wait for the button to be clicked *or* for a timer to expire? I'm glad you asked! D++ offers \ref dpp::when_any "when_any" which allows exactly that. It is a templated class that can take any number of awaitable objects and can be `co_await`-ed itself, will resume when the __first__ awaitable completes and return a \ref dpp::when_any::result "result" object that allows to retrieve which awaitable completed as well as its result, in a similar way as std::variant.

\include{cpp} coro_expiring_buttons.cpp

Any awaitable can be used with when_any, even dpp::task, dpp::coroutine, dpp::async. When the when_any object is destroyed, any of its awaitables with a cancel() method (for example \ref dpp::task::cancel "dpp::task") will have it called. With this you can easily make commands that ask for input in several steps, or maybe a timed text game, the possibilities are endless! Note that if the first awaitable completes with an exception, result.get will throw it.

\note when_any will try to construct awaitable objects from the parameter you pass it, which it will own. In practice this means you can only pass it <a href="https://www.learncpp.com/cpp-tutorial/value-categories-lvalues-and-rvalues/">temporary objects (rvalues)</a> as most of the coroutine-related objects in D++ are move-only.
