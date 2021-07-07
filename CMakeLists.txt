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

cmake_minimum_required (VERSION 3.12)

include(cmake/DetermineVersion.cmake)
include(cmake/colour.cmake)

# Initial Project Setup
project(libdpp
		VERSION ${DPP_VERSION}
		HOMEPAGE_URL https://github.com/brainboxdotcc/DPP/
		LANGUAGES CXX)

## CMake and Compiler Setup
set(CMAKE_CXX_STANDARD 17)                                      # Minimum C++ Version
set(CMAKE_CXX_EXTENSIONS OFF)                                   # True if compiler extensions are necessary. (Changes -std flag)
set(CMAKE_CXX_STANDARD_REQUIRED ON)                             # True to require minimum C++ version to compile
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/modules)  # Tell CMake where to access FindXXX.cmake files

# Project options
option(BUILD_SHARED_LIBS "Should the library be built shared (YES) or static (NO)" YES)
option(DPP_BUILD_TESTS   "Should testing be compiled?" FALSE)

# Project variable defaults
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Project Build Targets
if(BUILD_SHARED_LIBS)
	add_library(libdpp SHARED)
else()
	message("-- Building static library.")

	add_library(libdpp STATIC)
	set(OPENSSL_USE_STATIC_LIBS ON)
	set(sodium_USE_STATIC_LIBS ON)
	set(OPUS_USE_STATIC_LIBS TRUE)
endif()
target_include_directories(libdpp PUBLIC ${PROJECT_SOURCE_DIR}/include)

# OS Specific Initialization
if(WIN32)
	include(cmake/CMakeWindows.cmake)
elseif(APPLE)
	set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif()

# Find packages
find_package(Git QUIET)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(Sodium)
if(NOT WIN32) # These libs are found differently on windows
	find_package(ZLIB REQUIRED)
	find_package(Opus)
endif()

# Collect sources
file(GLOB sources CONFIGURE_DEPENDS src/dpp/*.cpp src/dpp/events/*.cpp)
target_sources(libdpp PRIVATE ${sources})

# Link available libraries
target_link_libraries(libdpp PUBLIC OpenSSL::SSL)
target_link_libraries(libdpp PRIVATE Threads::Threads)
target_link_libraries(libdpp PRIVATE ZLIB::ZLIB)

# Determine if voice support will be enabled
if(Sodium_FOUND AND Opus_FOUND)
	target_link_libraries(libdpp PRIVATE sodium)
	target_link_libraries(libdpp PRIVATE opus)

	message(STATUS "Detected ${Green}libsodium${ColourReset} and ${Green}libopus${ColourReset}. VOICE support will be ${Green}enabled${ColourReset}")
	target_compile_definitions(libdpp PRIVATE HAVE_VOICE)
else()
	message(STATUS "Could not detect ${Green}libsodium${ColourReset} and/or ${Green}libopus${ColourReset}. VOICE support will be ${Red}disabled${ColourReset}")
endif()

# Build tests
if(DPP_BUILD_TESTS)
	message(STATUS D++ Tests enabled. Adding test targets)
	add_executable(test src/test/test.cpp)
	target_link_libraries(test PRIVATE libdpp)
endif()

# CPack Initialization
include(cmake/CPackSetup.cmake)
include(CPack)

return()
###################################################################################
###################################################################################
###################################################################################
###################################################################################
#
#if(UNIX OR MSYS)
#	find_program(LDCONFIG_EXECUTABLE "ldconfig")
#endif()
#
#
#if(WIN32)
#	configure_file("${VCPKG_INSTALLATION_64_BITS_FOLDER}/bin/zlib1.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" COPYONLY)
#endif()
#
#if(NOT WIN32)
#	# Installation
#
#	include(GNUInstallDirs)
#	install(TARGETS dpp LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
#	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
#	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -DLDCONFIG_EXECUTABLE=${LDCONFIG_EXECUTABLE} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PostInstall.cmake)")
#endif()
