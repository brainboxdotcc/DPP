#
# D++ (DPP), The Lightweight C++ Discord Library
#
# Copyright 2021 Craig Edwards <support@brainbox.cc>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required (VERSION 3.16)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_VOICE_SUPPORT "Build voice support" ON)
option(RUN_LDCONFIG "Run ldconfig after installation" ON)
option(DPP_BUILD_TEST "Build the test program" ON)
option(DPP_NO_VCPKG "No VCPKG" OFF)
option(DPP_CORO "Experimental support for C++20 coroutines" OFF)

include(CheckCXXSymbolExists)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_definitions(DPP_BUILD)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/dpp/version.h" version_h)

if(NOT version_h MATCHES "DPP_VERSION_SHORT ([0-9][0-9])([0-9][0-9])([0-9][0-9])")
	message(FATAL_ERROR "Cannot get DPP_VERSION_SHORT from version.h")
endif()

math(EXPR DPP_VERSION_MAJOR "${CMAKE_MATCH_1}")
math(EXPR DPP_VERSION_MINOR "${CMAKE_MATCH_2}")
math(EXPR DPP_VERSION_PATCH "${CMAKE_MATCH_3}")

string(CONCAT DPP_VERSION "${DPP_VERSION_MAJOR}.${DPP_VERSION_MINOR}.${DPP_VERSION_PATCH}")

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake/")

if (DPP_NO_VCPKG)
	message("-- INFO: Explicitly disabling VCPKG as running inside the CI action.")
else()
	message("-- INFO: Using VCPKG if detected")
endif()

if (WIN32 AND NOT MINGW AND BUILD_SHARED_LIBS)
	message("-- INFO: Configuring .rc resource script")
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/dpp/dpp.rc.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/dpp/dpp.rc" NEWLINE_STYLE WIN32)
endif()

if (NOT DPP_NO_VCPKG AND EXISTS "${_VCPKG_ROOT_DIR}")
	set(PROJECT_NAME "dpp")
	project(
		"${PROJECT_NAME}"
		VERSION "${DPP_VERSION}"
		LANGUAGES CXX
		HOMEPAGE_URL "https://dpp.dev/"
		DESCRIPTION "An incredibly lightweight C++ Discord library."
	)
	add_subdirectory(library-vcpkg)	
else()
	set(PROJECT_NAME "libdpp")
	project(
		"${PROJECT_NAME}"
		VERSION "${DPP_VERSION}"
		LANGUAGES CXX
		HOMEPAGE_URL "https://dpp.dev/"
		DESCRIPTION "An incredibly lightweight C++ Discord library."
	)
	add_subdirectory(library)
endif()
