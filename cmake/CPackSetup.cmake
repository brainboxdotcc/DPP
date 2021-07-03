set(CPACK_PACKAGE_NAME   libdpp)            # Name of generated file
set(CPACK_PACKAGE_VENDOR Brainbox.cc)       # Maker of the application

install(DIRECTORY include DESTINATION .)    # Pack the include files
install(TARGETS dpp LIBRARY DESTINATION .)  # Pack the binary output

if(WIN32)
    set(CPACK_GENERATOR ZIP)
elseif(UNIX)
    set(CPACK_GENERATOR TGZ)
endif()