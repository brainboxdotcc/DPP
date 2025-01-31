\page using-coroutines Using Coroutines

\include{doc} coro_warn.dox

One of the most anticipated features of C++20 is the addition of coroutines: in short, they are functions that can be paused while waiting for data and resumed when that data is ready. They make asynchronous programs much easier to write, but they do come with additional dangers and subtleties.

* \subpage coro-introduction
* \subpage coro-simple-commands
* \subpage awaiting-events
* \subpage expiring-buttons

Coroutines require you to use C++20. You can do this by adding
- `-std=c++20` in your build command (before specifying files) if building manually, or,
- if using CMake, by adding `target_compile_features(my_program PUBLIC cxx_std_20)` in your `CMakeLists.txt`.

If you don't want to use Coroutines, You can either add
- `-DDPP_NO_CORO` in your build command, or, if using CMake,
- `target_compile_definitions(my_program PUBLIC DPP_NO_CORO)`.
- Additionally, you can build D++ without Coroutines with the same above.
