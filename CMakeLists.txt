cmake_minimum_required(VERSION 3.20)
set(PROJECT_NAME "dpp")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/dpp/version.h" version_h)

if(NOT version_h MATCHES "DPP_VERSION_SHORT ([0-9][0-9])([0-9][0-9])([0-9][0-9])")
	message(FATAL_ERROR "Cannot get DPP_VERSION_SHORT from version.h")
endif()

math(EXPR DPP_VERSION_MAJOR "${CMAKE_MATCH_1}")
math(EXPR DPP_VERSION_MINOR "${CMAKE_MATCH_2}")
math(EXPR DPP_VERSION_PATCH "${CMAKE_MATCH_3}")

string(CONCAT DPP_VERSION "${DPP_VERSION_MAJOR}.${DPP_VERSION_MINOR}.${DPP_VERSION_PATCH}")

project(
	"${PROJECT_NAME}"
	VERSION "${DPP_VERSION}"
	LANGUAGES CXX
	HOMEPAGE_URL "https://dpp.dev/"
	DESCRIPTION "An incredibly lightweight C++ Discord library"
)

set(LIB_NAME "${PROJECT_NAME}")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}")

add_subdirectory(Library)