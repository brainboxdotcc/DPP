vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO brainboxdotcc/DPP
	REF 3cb028e044d5bb709ddfcee65df1f6643cfc6c58
	SHA512 2635b167e2cf19bf423fbf17a06d3dd3e2f111de22ff36c453147bfebec11caac3706694b03b9aae692ed6a56bd97909a34a7d7b70998f57f740a171a66c9f4e
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
