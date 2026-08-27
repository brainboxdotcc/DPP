\page using_modules Using D++ with C++ modules

\include{doc} modules_warn.dox

D++ is offered as a C++ module, which offers improved compile times over a traditional header.

In order to enable support, you must use C++20 (or later) with any module-supporting compiler. To activate the feature, pass the `DPP_MODULES` flag to CMake. Ensure that the generated build system supports modules (for CMake, this is usually Ninja; note CMake does not currently support modules with Makefile).

The module interface unit is attached to the D++ library target itself, and `find_package` exports that target under a namespace. This means that your bot must link against `dpp::dpp`, and not a bare `dpp`:

~~~~~~~~~~~~~~cmake
# Module support requires CMake 3.28 or above.
cmake_minimum_required(VERSION 3.28)
project(discord-bot)

find_package(dpp REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)

# dpp::dpp carries the module interface unit along with it. A bare dpp does not!
target_link_libraries(${PROJECT_NAME} PRIVATE dpp::dpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	CXX_SCAN_FOR_MODULES ON
)
~~~~~~~~~~~~~~

\warning Writing a bare `dpp` instead of `dpp::dpp` will **not** give you an error from CMake. A name is only treated as a target if it contains `::`, so a plain `dpp` is quietly handed to the linker as `-ldpp` instead. Your build will then fail later on with an error saying that the module `dpp` cannot be found, because your compiler was never told where the module interface unit lives.

The other properties in the example above are just as important as the link line:

* `CXX_SCAN_FOR_MODULES` tells CMake to scan your sources for `import` statements. Without it, CMake never notices that your bot depends on the D++ module at all.
* We set `CXX_EXTENSIONS OFF`, as compilers will refuse to load a module with extensions enabled on another module without extensions enabled.

Once this is done, simply `import dpp;` and we're good to go!

\include{cpp} using_modules.cpp
