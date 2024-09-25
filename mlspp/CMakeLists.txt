cmake_minimum_required(VERSION 3.13)

project(mlspp
  VERSION 0.1
  LANGUAGES CXX
)

option(TESTING    "Build tests" OFF)
option(CLANG_TIDY "Perform linting with clang-tidy" OFF)
option(SANITIZERS "Enable sanitizers" OFF)
option(MLS_NAMESPACE_SUFFIX "Namespace Suffix for CXX and CMake Export")
option(DISABLE_GREASE "Disables the inclusion of MLS protocol recommended GREASE values" OFF)
option(REQUIRE_BORINGSSL "Require BoringSSL instead of OpenSSL" OFF)

if(MLS_NAMESPACE_SUFFIX)
    set(MLS_CXX_NAMESPACE "mls_${MLS_NAMESPACE_SUFFIX}" CACHE STRING "Top-level Namespace for CXX")
    set(MLS_EXPORT_NAMESPACE "MLSPP${MLS_NAMESPACE_SUFFIX}" CACHE STRING "Namespace for CMake Export")
else()
    set(MLS_CXX_NAMESPACE "../include/dpp/mlspp/mls" CACHE STRING "Top-level Namespace for CXX")
    set(MLS_EXPORT_NAMESPACE "MLSPP" CACHE STRING "Namespace for CMake Export")
endif()
message(STATUS "CXX Namespace: ${MLS_CXX_NAMESPACE}")
message(STATUS "CMake Export Namespace: ${MLS_EXPORT_NAMESPACE}")


###
### Global Config
###
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

configure_file(
  "cmake/namespace.h.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/namespace.h"
  @ONLY
)

include(CheckCXXCompilerFlag)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  add_compile_options(-Wall -pedantic -Wextra -Werror -Wmissing-declarations)
elseif(MSVC)
  add_compile_options(/W4 /WX)
  add_definitions(-DWINDOWS)

  # MSVC helpfully recommends safer equivalents for things like
  # getenv, but they are not portable.
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)  
endif()

if("$ENV{MACOSX_DEPLOYMENT_TARGET}" STREQUAL "10.11")
  add_compile_options(-DVARIANT_COMPAT)
endif()

if (DISABLE_GREASE)
  add_compile_options(-DDISABLE_GREASE)
endif ()

###
### Dependencies
###

# Configure vcpkg to only build release libraries
set(VCPKG_BUILD_TYPE release)

# External libraries
find_package(OpenSSL REQUIRED)
if ( OPENSSL_FOUND )
    find_path(BORINGSSL_INCLUDE_DIR openssl/is_boringssl.h HINTS ${OPENSSL_INCLUDE_DIR} NO_DEFAULT_PATH)

    if (BORINGSSL_INCLUDE_DIR)
      message(STATUS "Found OpenSSL includes are for BoringSSL")
      
      add_compile_definitions(WITH_BORINGSSL)

      if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-Wno-gnu-anonymous-struct -Wno-nested-anon-types)
      endif ()

      file(STRINGS "${OPENSSL_INCLUDE_DIR}/openssl/crypto.h" boringssl_version_str
           REGEX "^#[\t ]*define[\t ]+OPENSSL_VERSION_TEXT[\t ]+\"OpenSSL ([0-9])+\\.([0-9])+\\.([0-9])+ .+")
      
      string(REGEX REPLACE "^.*OPENSSL_VERSION_TEXT[\t ]+\"OpenSSL ([0-9]+\\.[0-9]+\\.[0-9])+ .+$"
             "\\1" OPENSSL_VERSION "${boringssl_version_str}")

    elseif (REQUIRE_BORINGSSL)
      message(FATAL_ERROR "BoringSSL required but not found")
    endif ()

    if (${OPENSSL_VERSION} VERSION_GREATER_EQUAL 3)
      add_compile_definitions(WITH_OPENSSL3)
    elseif(${OPENSSL_VERSION} VERSION_LESS 1.1.1)
      message(FATAL_ERROR "OpenSSL 1.1.1 or greater is required")
    endif()
    message(STATUS "OpenSSL Found: ${OPENSSL_VERSION}")
    message(STATUS "OpenSSL Include: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL Libraries: ${OPENSSL_LIBRARIES}")
else()
  message(FATAL_ERROR "No OpenSSL library found")
endif()

# Internal libraries
add_subdirectory(lib)

# Third-Party libraries in tree
add_subdirectory(third_party)


###
### Library Config
###

set(LIB_NAME "${PROJECT_NAME}")

file(GLOB_RECURSE LIB_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(${LIB_NAME} STATIC ${LIB_HEADERS} ${LIB_SOURCES})
add_dependencies(${LIB_NAME} bytes tls_syntax hpke)
target_link_libraries(${LIB_NAME} bytes tls_syntax hpke)
target_include_directories(${LIB_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
  PRIVATE
    ${OPENSSL_INCLUDE_DIR}
)

install(TARGETS ${LIB_NAME} EXPORT mlspp-targets)


###
### Exports
###
set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
export(
  EXPORT
    mlspp-targets
  NAMESPACE
    ${MLS_EXPORT_NAMESPACE}::
  FILE
    ${MLS_EXPORT_NAMESPACE}Targets.cmake)
export(PACKAGE ${MLS_EXPORT_NAMESPACE})

configure_package_config_file(cmake/config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${MLS_EXPORT_NAMESPACE}Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/${MLS_EXPORT_NAMESPACE}
  NO_SET_AND_CHECK_MACRO)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${MLS_EXPORT_NAMESPACE}ConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

###
### Install
###

install(
  EXPORT
    mlspp-targets
  NAMESPACE
    ${MLS_EXPORT_NAMESPACE}::
  FILE
    ${MLS_EXPORT_NAMESPACE}Targets.cmake
  DESTINATION
    ${CMAKE_INSTALL_DATADIR}/${MLS_EXPORT_NAMESPACE})

install(
  DIRECTORY
        include
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${MLS_EXPORT_NAMESPACE}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${MLS_EXPORT_NAMESPACE}ConfigVersion.cmake
  DESTINATION
    ${CMAKE_INSTALL_DATADIR}/${MLS_EXPORT_NAMESPACE})

install(
  FILES
        LICENSE
  DESTINATION
    ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
  RENAME
    copyright)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/bytes/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/hpke/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/mls_vectors/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/tls_syntax/include")
