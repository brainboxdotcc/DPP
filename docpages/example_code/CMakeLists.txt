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

# Example programs test compilation
# This build script is executed by a GitHub action to ensure all example
# programs compile correctly. It does not attempt to run them, as there
# is no way to know if the program successfully did its thing, plus
# examples do not have a valid token. This build script assumes the
# following system dependencies are available:
#
# g++-12 or later
# liboggz-dev
# libmpg123-dev
# dpp latest master with -DDPP_CORO=ON installed sytemwide

cmake_minimum_required (VERSION 3.16)
project(documentation_tests)

string(ASCII 27 Esc)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDPP_CORO -std=c++20 -pthread -O0 -fPIC -rdynamic -DFMT_HEADER_ONLY -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")

file(GLOB example_list ./*.cpp)
foreach (example ${example_list})
	get_filename_component(examplename ${example} NAME)
	message(STATUS "Found example '${Esc}[1;34m${examplename}${Esc}[m'")
	add_executable(${examplename}_out ${example})
	target_link_libraries(${examplename}_out dl dpp mpg123 oggz ogg opusfile opus)
	include_directories(/usr/include/opus)
endforeach(example)
