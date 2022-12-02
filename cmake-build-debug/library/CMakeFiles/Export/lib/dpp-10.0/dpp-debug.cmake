#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dpp::dpp" for configuration "Debug"
set_property(TARGET dpp::dpp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(dpp::dpp PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/dpp-10.0/libdpp.so.10.0.22"
  IMPORTED_SONAME_DEBUG "libdpp.so.10.0.22"
  )

list(APPEND _IMPORT_CHECK_TARGETS dpp::dpp )
list(APPEND _IMPORT_CHECK_FILES_FOR_dpp::dpp "${_IMPORT_PREFIX}/lib/dpp-10.0/libdpp.so.10.0.22" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
