vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO brainboxdotcc/DPP
    REF 02a16bc415ead32abc59fca8579c6611ce5e8ed6
    SHA512  86d8abfd0f6796943634d104cd3298af4e71e9bdc9e7bf8fbf7b082e0dddb964b5f53aa0edff442060d7ec19dcf9d96e526b8dddee19388d75935e0bbd217a05
    HEAD_REF master
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

