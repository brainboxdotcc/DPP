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

add_compile_definitions(HAVE_VOICE)

file(GLOB THE_SOURCES "${DPP_ROOT_PATH}/src/dpp/events/*.cpp" "${DPP_ROOT_PATH}/src/dpp/voice/enabled/*.cpp" "${DPP_ROOT_PATH}/src/dpp/dave/*.cpp" "${DPP_ROOT_PATH}/src/dpp/cluster/*.cpp" "${DPP_ROOT_PATH}/src/dpp/*.cpp" "${DPP_ROOT_PATH}/src/dpp/*.rc")

set(LIB_NAME "${PROJECT_NAME}")

add_library("${LIB_NAME}" SHARED "${THE_SOURCES}")
if(NOT WIN32)
	find_package(Threads REQUIRED)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/epoll.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/kqueue.cmake")
check_epoll(HAS_EPOLL)
check_kqueue(HAS_KQUEUE)
if (HAS_EPOLL)
	message("-- Building with ${Green}epoll socket engine${ColourReset} -- ${Green}good!${ColourReset}")
	target_sources("${LIB_NAME}" PRIVATE "${DPP_ROOT_PATH}/src/dpp/socketengines/epoll.cpp")
elseif (HAS_KQUEUE)
	message("-- Building with ${Green}kqueue socket engine${ColourReset} -- ${Green}good!${ColourReset}")
	target_sources("${LIB_NAME}" PRIVATE "${DPP_ROOT_PATH}/src/dpp/socketengines/kqueue.cpp")
else()
	message("-- Building with ${Green}poll socket engine${ColourReset} -- ${Red}meh${ColourReset}!")
	target_sources("${LIB_NAME}" PRIVATE "${DPP_ROOT_PATH}/src/dpp/socketengines/poll.cpp")
endif()


add_library("${PROJECT_NAME}::${LIB_NAME}" ALIAS "${LIB_NAME}")

if(${AVX_TYPE} STREQUAL "OFF")
	include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/DetectArchitecture.cmake")
else()
	message("-- AVX type overridden by configuration: ${AVX_TYPE}")
endif()
STRING(REPLACE "AVX" "" AVX_TYPE ${AVX_TYPE})
add_compile_definitions(AVX_TYPE=${AVX_TYPE})
add_compile_options(${AVX_FLAG})

target_compile_definitions(
	"${LIB_NAME}" PUBLIC
	"DPP_BUILD"
)

if(WIN32)
	add_compile_definitions(OPENSSL_SYS_WIN32)
	add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
	add_compile_definitions(WIN32_LEAN_AND_MEAN)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	add_compile_definitions(_CRT_NONSTDC_NO_DEPRECATE)
	# Fake an ssl version number to satisfy MLSPP
	set(OPENSSL_VERSION "1.1.1f")
endif()



target_compile_options(
	"${LIB_NAME}"
	PUBLIC
		"$<$<PLATFORM_ID:Windows>:/bigobj;/Zc:preprocessor>"
	PRIVATE
		"$<$<PLATFORM_ID:Windows>:$<$<CONFIG:Debug>:/sdl;/Od;/DEBUG;/MP;/DFD_SETSIZE=1024>>"
		"$<$<PLATFORM_ID:Windows>:$<$<CONFIG:Release>:/O2;/Oi;/Oy;/GL;/Gy;/sdl;/MP;/DFD_SETSIZE=1024>>"
		"$<$<PLATFORM_ID:Linux>:$<$<CONFIG:Debug>:-fPIC;-Wall;-Wempty-body;-Wno-deprecated-declarations;-Wno-psabi;-Wunknown-pragmas;-Wignored-qualifiers;-Wimplicit-fallthrough;-Wmissing-field-initializers;-Wsign-compare;-Wtype-limits;-Wuninitialized;-Wshift-negative-value;-pthread;-g;-Og;-fPIC>>"
		"$<$<PLATFORM_ID:Linux>:$<$<CONFIG:Release>:-fPIC;-Wall;-Wempty-body;-Wno-deprecated-declarations;-Wno-psabi;-Wunknown-pragmas;-Wignored-qualifiers;-Wimplicit-fallthrough;-Wmissing-field-initializers;-Wsign-compare;-Wtype-limits;-Wuninitialized;-Wshift-negative-value;-pthread;-O3;-fPIC>>"
		"${AVX_FLAG}"
)

target_compile_features(
	"${LIB_NAME}" PUBLIC
	"cxx_std_17" "cxx_constexpr" "cxx_auto_type"
	"cxx_defaulted_functions" "cxx_deleted_functions"
	"cxx_final" "cxx_lambdas" "cxx_override" "cxx_thread_local"
	"cxx_variadic_templates" "cxx_attribute_deprecated"
	"cxx_enum_forward_declarations"
)

target_include_directories(
	"${LIB_NAME}" PUBLIC
	"$<BUILD_INTERFACE:${DPP_ROOT_PATH}/include>"
	"$<INSTALL_INTERFACE:include>"
)

find_package(OpenSSL REQUIRED)
add_subdirectory("${DPP_ROOT_PATH}/mlspp" "mlspp")
include_directories("${DPP_ROOT_PATH}/mlspp/include")
include_directories("${DPP_ROOT_PATH}/mlspp/lib/bytes/include")
include_directories("${DPP_ROOT_PATH}/mlspp/lib/hpke/include")

set_target_properties(
	"${LIB_NAME}" PROPERTIES
	OUTPUT_NAME "dpp"
	CXX_STANDARD_REQUIRED ON
)

target_link_options(
	"${LIB_NAME}" PUBLIC
	"$<$<PLATFORM_ID:Windows>:$<$<CONFIG:Debug>:/DEBUG>>"
)

find_package(nlohmann_json CONFIG REQUIRED)
if (DPP_TEST_VCPKG)
	include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/FindOpus.cmake")
	find_package(Opus REQUIRED)
else()
	find_package(Opus CONFIG REQUIRED)
endif()
find_package(ZLIB REQUIRED)

target_link_libraries(
	"${LIB_NAME}" PUBLIC
	$<$<TARGET_EXISTS:nlohmann_json::nlohmann_json>:nlohmann_json::nlohmann_json>
	$<$<TARGET_EXISTS:OpenSSL::SSL>:OpenSSL::SSL>
	$<$<TARGET_EXISTS:OpenSSL::Crypto>:OpenSSL::Crypto>
	$<$<TARGET_EXISTS:Opus::opus>:Opus::opus>
	$<$<TARGET_EXISTS:ZLIB::ZLIB>:ZLIB::ZLIB>
	$<$<TARGET_EXISTS:Threads::Threads>:Threads::Threads>
)

# Private statically linked dependencies
target_link_libraries(
	${LIB_NAME} PRIVATE
	mlspp
	mls_vectors
	bytes
	tls_syntax
	hpke
)

set(CONFIG_FILE_NAME "${PROJECT_NAME}Config.cmake")
set(EXPORTED_TARGETS_NAME "${PROJECT_NAME}Targets")
set(EXPORTED_TARGETS_FILE_NAME "${EXPORTED_TARGETS_NAME}.cmake")
set(EXPORTED_TARGETS_FILE_PATH "share/dpp/${EXPORTED_TARGETS_FILE_NAME}")
set(RELEASE_PDB_FILE_PATH "bin/dpp.pdb")
set(DEBUG_PDB_FILE_PATH "debug/bin/dpp.pdb")

include(CMakePackageConfigHelpers)
configure_package_config_file(
	"${DPP_ROOT_PATH}/cmake/${CONFIG_FILE_NAME}.in"
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
	DIRECTORY "${DPP_ROOT_PATH}/include/"
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
