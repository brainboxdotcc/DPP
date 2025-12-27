add_library(dpp_module)

file(GLOB DPP_MODULE_SOURCES "*.cppm")

target_sources(dpp_module
    PUBLIC
        FILE_SET CXX_MODULES
        FILES ${DPP_MODULE_SOURCES}
)

target_compile_features(dpp_module PUBLIC cxx_std_20)

target_include_directories(dpp_module PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_library(dpp::module ALIAS dpp_module)

# Installation
install(TARGETS dpp_module
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dpp/src/modules
)
