vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO brainboxdotcc/DPP
	REF 6b905ce20dfbf5e982f8e7ca3c6b94d29373a488
	SHA512 6ca2bd5e7b2d8d284604b36864d5dac7ee9ebb9025700defd75d9e7fcd856377e86e813fa52151bd144e18a57531345d9bb226db02d64d5b92e943c31c0a1892
	HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(NO_PREFIX_CORRECTION)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share/dpp")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin" "${CURRENT_PACKAGES_DIR}/debug/bin")
endif()

file(
	INSTALL "${SOURCE_PATH}/LICENSE"
	DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
	RENAME copyright
)
