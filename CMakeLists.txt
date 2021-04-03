#
# Sporks, the learning, scriptable Discord bot!
#
# Copyright 2019 Craig Edwards <support@sporks.gg>
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

cmake_minimum_required (VERSION 3.6)


project(test)

aux_source_directory("src" coresrc)

add_executable(test ${coresrc})

string(ASCII 27 Esc)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

target_compile_features(test PRIVATE cxx_std_17)

include_directories( "include" )

target_link_libraries(test)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/deps/spdlog/CMakeLists.txt" OR NOT EXISTS "${PROJECT_SOURCE_DIR}/deps/json/CMakeLists.txt")
    message(FATAL_ERROR "The submodules were not downloaded! Please update submodules and try again.")
endif()

add_subdirectory(deps/spdlog)
add_subdirectory(deps/json)

include_directories("deps/spdlog/include")
include_directories("deps/json/single_include")

execute_process(COMMAND "g++ -c deps/json/single_include/nlohmann/json.hpp -o deps/json/single_include/nlohmann/json.hpp.gch")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -g -O2 -fPIC -rdynamic")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2")

set (modules_dir "src")
file(GLOB subdirlist ${modules_dir}/dpp)
foreach (fullmodname ${subdirlist})
        get_filename_component(modname ${fullmodname} NAME)
        set (modsrc "")
	file(GLOB modsrc ${modules_dir}/dpp/*.cpp ${modules_dir}/dpp/events/*.cpp)
        add_library(${modname} SHARED ${modsrc})
if (WIN32)
	target_link_libraries(${modname} PRIVATE libssl64MTd crypto64MTd nlohmann_json::nlohmann_json)
else (WIN32)
	target_link_libraries(${modname} PRIVATE ssl crypto nlohmann_json::nlohmann_json)
endif (WIN32)
endforeach(fullmodname)

target_compile_features(dpp PRIVATE cxx_std_17)
target_link_libraries(test dpp spdlog)

