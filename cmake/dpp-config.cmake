# dpp-config.cmake - package configuration file

## Get current filesystem path (will a prefixed by where this package was installed)
get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

## Make bundled find-modules (e.g. FindFilesystem) discoverable by find_dependency below.
list(APPEND CMAKE_MODULE_PATH "${SELF_DIR}")

include(CMakeFindDependencyMacro)

## Set OpenSSl directory for macos. It is also in our main CMakeLists.txt, but this file is independent from that.
if(APPLE)
	if(CMAKE_APPLE_SILICON_PROCESSOR)
		set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl")
	else()
		set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
	endif()
endif()

## Search for libdpp dependencies
find_dependency(OpenSSL REQUIRED)

## When D++ is built with C++20 module support the exported module links against the
## std::filesystem imported target on toolchains that need a separate <filesystem>
## library, so consumers must be able to resolve it too. Harmless otherwise.
find_dependency(Filesystem REQUIRED)

## Use this directory to include dpp which has the rest of the project targets
include(${SELF_DIR}/dpp.cmake)
