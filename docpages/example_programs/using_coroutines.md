\page using-coroutines Using Coroutines

\include{doc} coro_warn.dox

One of the most anticipated features of C++20 is the addition of coroutines: in short, they are functions that can be paused while waiting for data and resumed when that data is ready. They make asynchronous programs much easier to write, but they do come with additional dangers and subtleties.

* \subpage coro-introduction
* \subpage coro-simple-commands
* \subpage awaiting-events
* \subpage expiring-buttons

Coroutines are currently disabled by default; to use them you will need to build D++ \ref install-from-source "from source" and use the option `-DDPP_CORO=on` in your CMake command.
Your application also needs to enable C++20 and define DPP_CORO, by using:
- `-std=c++20 -DDPP_CORO` in your build command if building manually, or
- if using CMake, add `target_compile_definitions(my_program PUBLIC DPP_CORO)` and `target_compile_features(my_program PUBLIC cxx_std_20)`.
