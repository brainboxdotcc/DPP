#
#	DiscordCoreAPI, A bot library for Discord, written in C++, and featuring explicit multithreading through the usage of custom, asynchronous C++ CoRoutines.
#
#	Copyright 2021, 2022 Chris M. (RealTimeChris)
#
#	This file is part of DiscordCoreAPI.
#	DiscordCoreAPI is free software: you can redistribute it and/or modify it under the terms of the GNU
#	General Public License as published by the Free Software Foundation, either version 3 of the License,
#	or (at your option) any later version.
#	DiscordCoreAPI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
#	even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#	You should have received a copy of the GNU General Public License along with DiscordCoreAPI.
#	If not, see <https://www.gnu.org/licenses/>.
#
# CMakeLists.txt - The CMake script for building this library.
# May 13, 2021
# https://discordcoreapi.com

include(GenerateProductVersion)
generate_product_version(
	PRODUCT_INFO
	NAME "${LIB_NAME}"
	FILE_DESCRIPTION "${DESCRIPTION}"
	ORIGINAL_FILENAME "dpp.dll"
	COMPANY_NAME "brainboxdotcc"
	VERSION_MAJOR "10"
	VERSION_MINOR "13"
	VERSION_PATCH "0"
	VERSION_REVISION "0"
)

if(MSVC)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /Od /DEBUG /sdl /MP /DFD_SETSIZE=1024 /Zc:preprocessor")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /O2 /Oi /Oy /GL /Gy /sdl /MP /DFD_SETSIZE=1024 /Zc:preprocessor")
	endif()
	string(REGEX REPLACE "/W[1|2|3|4]" "/W3" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wempty-body -Wunknown-pragmas -Wignored-qualifiers -Wimplicit-fallthrough -Wmissing-field-initializers -Wsign-compare -Wtype-limits -Wuninitialized -Wshift-negative-value -pthread -g -Og -fPIC")
	else()
		if (BUILD_O3)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wempty-body -Wunknown-pragmas -Wignored-qualifiers -Wimplicit-fallthrough -Wmissing-field-initializers -Wsign-compare -Wtype-limits -Wuninitialized -Wshift-negative-value -pthread -O3 -fPIC")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wempty-body -Wunknown-pragmas -Wignored-qualifiers -Wimplicit-fallthrough -Wmissing-field-initializers -Wsign-compare -Wtype-limits -Wuninitialized -Wshift-negative-value -pthread -Os -fPIC")
		endif()
	endif()

	if (NOT MINGW)
		add_link_options("-rdynamic")
	endif ()
endif()

file(GLOB modsrc "${CMAKE_SOURCE_DIR}/src/dpp/*.cpp" "${CMAKE_SOURCE_DIR}/src/dpp/events/*.cpp" "${CMAKE_SOURCE_DIR}/src/dpp/cluster/*.cpp")

if(WIN32)
	add_library("${LIB_NAME}" SHARED "${modsrc}" "${PRODUCT_INFO}")
else()
	add_library("${LIB_NAME}" STATIC "${modsrc}")
	find_package(Threads REQUIRED)
endif()

target_include_directories(
	"${LIB_NAME}" PRIVATE
	"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
	"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/dpp>"
	"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/dpp/fmt>"
	"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/dpp/nlohmann>"
)

add_library("${PROJECT_NAME}::${LIB_NAME}" ALIAS "${LIB_NAME}")
target_compile_definitions("${LIB_NAME}" PUBLIC "DPP_BUILD")
target_compile_features("${LIB_NAME}" PRIVATE cxx_std_17)
target_compile_features("${LIB_NAME}" PRIVATE cxx_constexpr)
target_compile_features("${LIB_NAME}" PRIVATE cxx_auto_type)
target_compile_features("${LIB_NAME}" PRIVATE cxx_defaulted_functions)
target_compile_features("${LIB_NAME}" PRIVATE cxx_deleted_functions)
target_compile_features("${LIB_NAME}" PRIVATE cxx_final)
target_compile_features("${LIB_NAME}" PRIVATE cxx_lambdas)
target_compile_features("${LIB_NAME}" PRIVATE cxx_override)
target_compile_features("${LIB_NAME}" PRIVATE cxx_thread_local)
target_compile_features("${LIB_NAME}" PRIVATE cxx_variadic_templates)
target_compile_features("${LIB_NAME}" PRIVATE cxx_attribute_deprecated)

file(GLOB HEADERS01 CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/dpp/*.h")
file(GLOB HEADERS02 CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/dpp/fmt/*.h")
file(GLOB HEADERS03 CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/dpp/nlohmann/*.hpp")

set(HEADERS "${HEADERS01};${HEADERS02};${HEADERS03}")

set_target_properties(
	"${LIB_NAME}" PROPERTIES
	OUTPUT_NAME "dpp"
	CXX_STANDARD_REQUIRED ON
)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Opus CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

target_link_libraries(
	"${LIB_NAME}" PUBLIC
	$<$<TARGET_EXISTS:nlohmann_json::nlohmann_json>:nlohmann_json::nlohmann_json>
	$<$<TARGET_EXISTS:OpenSSL::SSL>:OpenSSL::SSL>
	$<$<TARGET_EXISTS:OpenSSL::Crypto>:OpenSSL::Crypto>
	$<$<TARGET_EXISTS:Opus::opus>:Opus::opus>
	$<$<TARGET_EXISTS:unofficial-sodium::sodium>:unofficial-sodium::sodium>
	$<$<TARGET_EXISTS:ZLIB::ZLIB>:ZLIB::ZLIB>
	$<$<TARGET_EXISTS:Threads::Threads>:Threads::Threads>
)

target_compile_features(
	"${LIB_NAME}" PUBLIC
	"cxx_std_17"
)

target_compile_options(
	"${LIB_NAME}" PUBLIC
	"$<$<NOT:$<PLATFORM_ID:Linux>>:/bigobj>"
	"$<$<PLATFORM_ID:Windows>:/MP>"
	"$<$<PLATFORM_ID:Linux>:-O3>"
)

target_link_options(
	"${LIB_NAME}" PUBLIC
	"$<$<PLATFORM_ID:Windows>:/DEBUG>"
)

set(CONFIG_FILE_NAME "${PROJECT_NAME}Config.cmake")
set(EXPORTED_TARGETS_NAME "${PROJECT_NAME}Targets")
set(EXPORTED_TARGETS_FILE_NAME "${EXPORTED_TARGETS_NAME}.cmake")
set(EXPORTED_TARGETS_FILE_PATH "share/dpp/${EXPORTED_TARGETS_FILE_NAME}")
set(RELEASE_PDB_FILE_PATH "bin/dpp.pdb")
set(DEBUG_PDB_FILE_PATH "debug/bin/dpp.pdb")

include(CMakePackageConfigHelpers)
configure_package_config_file(
	"${CMAKE_SOURCE_DIR}/cmake/${CONFIG_FILE_NAME}.in"
	"${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_FILE_NAME}"
	INSTALL_DESTINATION "share/dpp"
	PATH_VARS
	EXPORTED_TARGETS_FILE_PATH
	RELEASE_PDB_FILE_PATH
	DEBUG_PDB_FILE_PATH
)

set(VERSION_FILE_NAME "${PROJECT_NAME}ConfigVersion.cmake")

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/${VERSION_FILE_NAME}"
	VERSION "${PRODUCT_VERSION}"
	COMPATIBILITY AnyNewerVersion
)

install(
	DIRECTORY "${CMAKE_SOURCE_DIR}/include/"
	DESTINATION "include"
)

install(
	FILES
	"${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_FILE_NAME}"
	"${CMAKE_CURRENT_BINARY_DIR}/${VERSION_FILE_NAME}"
	DESTINATION "share/dpp"
)

install(
	TARGETS "${LIB_NAME}"
	EXPORT "${EXPORTED_TARGETS_NAME}"
	RUNTIME DESTINATION "$<$<NOT:$<PLATFORM_ID:Linux>>:$<IF:$<CONFIG:Debug>,${DEBUG_PREFIX}bin,bin>>"
	ARCHIVE DESTINATION "$<IF:$<CONFIG:Debug>,${DEBUG_PREFIX}lib,lib>"
)

install(
	EXPORT "${EXPORTED_TARGETS_NAME}"
	FILE "${EXPORTED_TARGETS_FILE_NAME}"
	NAMESPACE "${PROJECT_NAME}::"
	DESTINATION "share/dpp"
)

if (WIN32)
	install(
		FILES 
		"$<TARGET_PDB_FILE:${LIB_NAME}>"
		DESTINATION "$<IF:$<CONFIG:Debug>,${DEBUG_PREFIX}bin,bin>"
		OPTIONAL
	)
endif()
