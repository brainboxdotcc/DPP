vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO brainboxdotcc/DPP
	REF 0d437cda621ed5da74628755a15af59f7df2d9a5
	SHA512 55591743d385af31070455cc3ca65da321bfcea270b067c6d08d43342eb2e75651a21e77c80f2e0e8dc12a3665673f250b08d2e6dea47324c2dad64edde532dc
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
