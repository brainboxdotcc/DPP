cmake_minimum_required(VERSION 3.30)

if (NOT MSVC OR CLANG_CL)
	message(FATAL_ERROR "Building the STD module strictly requires MSVC")
endif ()
if (DPP_STD_MODULE_LOCATION STREQUAL "")
	if (WIN32)
		set(DPP_LINK_STD_MODULE on)
		set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
		file(COPY
			"$ENV{VCToolsInstallDir}/modules/std.ixx"
			DESTINATION
			${PROJECT_BINARY_DIR}/stdxx
		)

		add_library(dppstdmodule)
		target_sources(dppstdmodule PUBLIC FILE_SET CXX_MODULES FILES "${PROJECT_BINARY_DIR}/stdxx/std.ixx")
		target_compile_features(dppstdmodule PUBLIC cxx_std_23)

		set(DPP_STD_MODULE_LOCATION "$<TARGET_FILE:dppstdmodule>")
		link_libraries(dppstdmodule)
	endif()
else()
	if (WIN32)
		add_compile_options("/reference \"${DPP_STD_MODULE_LOCATION}\"")
	endif ()
endif()
