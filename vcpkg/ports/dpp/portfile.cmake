vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO brainboxdotcc/DPP
    REF 9cb05c2cb0ee03882119cd5a2164849e1b8b6819
    SHA512  3bcd40f099aae7b95eec8a55e065e0ca7816e737d3a24b9c275dd28160c4ed555f3cdabbda697748478f6ae73b9a22c62219274c401cb9a2a1814d98c568c4ae
    HEAD_REF master
    PATCHES
        make-pkgconfig-required.patch
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    DISABLE_PARALLEL_CONFIGURE
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

file(COPY "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

