# Configure what will be included in a project output
## Export related directories
set(DPP_EXPORT_NAME libdpp)
set(DPP_VERSIONED ${DPP_EXPORT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
set(DPP_VERSION_FILE ${PROJECT_BINARY_DIR}/${DPP_EXPORT_NAME}-config-version.cmake)
set(DPP_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/${DPP_VERSIONED})
set(DPP_INSTALL_LIBRARY_DIR ${CMAKE_INSTALL_LIBDIR}/${DPP_VERSIONED})

## Pack the binary output
install(TARGETS libdpp
        EXPORT ${DPP_EXPORT_NAME}
        LIBRARY DESTINATION  ${DPP_INSTALL_LIBRARY_DIR}
        ARCHIVE DESTINATION  ${DPP_INSTALL_LIBRARY_DIR}
        RUNTIME DESTINATION  ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${DPP_INSTALL_INCLUDE_DIR})

## Allow for a specific version to be chosen in the `find_package` command
include(CMakePackageConfigHelpers)
write_basic_package_version_file(${DPP_VERSION_FILE}
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

## Package the include headers (the trailing slash is important, otherwise
## the include folder will be copied, instead of it's contents)
install(DIRECTORY include/ DESTINATION ${DPP_INSTALL_INCLUDE_DIR})

## Include the file which allows `find_package(libdpp)` to function.
install(FILES cmake/libdpp-config.cmake ${DPP_VERSION_FILE} DESTINATION ${DPP_INSTALL_LIBRARY_DIR})

## Export the targets to allow other projects to easily include this project
install(EXPORT ${DPP_EXPORT_NAME} DESTINATION ${DPP_INSTALL_LIBRARY_DIR} NAMESPACE dpp::)

# Prepare information for packaging into .zip, .deb, .rpm
## Project installation metadata
set(CPACK_PACKAGE_NAME   libdpp)            # Name of generated file
set(CPACK_PACKAGE_VENDOR Brainbox.cc)       # Maker of the application

## Select generated based on what operating system
if(WIN32)
    set(CPACK_GENERATOR ZIP)
elseif(UNIX)
    set(CPACK_GENERATOR TGZ)
endif()