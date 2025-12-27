\page using_modules Using D++ with C++ modules

D++ is offered as a C++ module, which offers improved compile times over a traditional header.

In order to enable support, you should be using C++20+ with any module-supporting compiler. To activate the feature, pass the `DPP_MODULES` flag to CMake. Ensure that the generated build system supports modules (for CMake, this is usually Ninja).

Once this is done, simply `import dpp;` and we're good to go!

\include{cpp} using_modules.cpp
