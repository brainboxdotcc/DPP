# Tell Visual Studio what project to run by default
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT libdpp)

# Prepare opus on windows
include(cmake/PrepareOpusWindows.cmake)

# Precompiled 64bit zlib added from https://www.bruot.org/hp/libraries/
add_library(zlib INTERFACE)
add_library(ZLIB::ZLIB ALIAS zlib) # Added to keep a single target_link_libraries for both windows and linux
target_include_directories(zlib INTERFACE ${PROJECT_SOURCE_DIR}/libs/zlib/include)
file(GLOB zlib_libs ${PROJECT_SOURCE_DIR}/libs/zlib/lib/*.lib)
target_link_libraries(zlib INTERFACE ${zlib_libs})
set(Opus_FOUND TRUE) # Set manually since we know we have it, and the find_package command won't run

# Precompiled 32 and 64bit sodium added from https://download.libsodium.org/libsodium/releases/
set(sodium_DIR ${PROJECT_SOURCE_DIR}/libs/libsodium)

# Windows specific compile definitions and options
target_compile_definitions(libdpp PRIVATE
	OPENSSL_SYS_WIN32
	WIN32_LEAN_AND_MEAN
	_CRT_SECURE_NO_WARNINGS
	_CRT_NONSTDC_NO_DEPRECATE
	_WINSOCK_DEPRECATED_NO_WARNINGS
)


if(MSVC)
	target_compile_options(libdpp PRIVATE
		# Universal Options
		/W4	   					  # Display up to level 4 warnings
		/sdl					  # Enable additional security checks
		/MP						  # Build with Multiple Processes

		# Debug mode only options
		$<$<CONFIG:Debug>:/DEBUG> # Generate Debug Info
		$<$<CONFIG:Debug>:/Zi>    # Generate PDB file
		$<$<CONFIG:Debug>:/Od>    # Disable Optimization

		# Release mode only options
		$<$<NOT:$<CONFIG:Debug>>:/O2>  # Optimize for speed (Equivalent to /Og /Oi /Ot /Oy /Ob2 /GF /Gy)
		$<$<NOT:$<CONFIG:Debug>>:/ZI>  # Create Debug Information (Forces /Gy and /FC)
		$<$<NOT:$<CONFIG:Debug>>:/GL>  # Whole program optimization
	)
endif()