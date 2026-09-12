#pragma once

// Windows	MSVC	cl			_WIN32 _MSC_VER
// Windows	LLVM	clang-cl	_WIN32 _MSC_VER __clang__
// Windows	UCRT64	g++			_WIN32 __MINGW32__ __GNUC__
// Windows	CLANG64	clang++		_WIN32 __MINGW32__ __GNUC__ __clang__
// Linux	GCC		g++			__linux__ __GNUC__
// Linux	Clang	clang++		__linux__ __GNUC__ __clang__

#if defined(_WIN32)
#	define DPP_USE_WINDOWS
#elif defined(__linux__) || defined(__linux) || defined(linux)
#	define DPP_USE_LINUX
#elif defined(__unix__) || defined(__unix) || defined(unix)
#	define DPP_USE_UNIX
#elif defined(__APPLE__) || defined(__MACH__)
#	define DPP_USE_MACOS
#else
#	error "Unknown platform"
#endif

#ifdef _MSC_VER
#	define DPP_USE_MSVC
#	ifdef __clang__
#		define DPP_USE_CLANG
#	endif
#elif defined(__GNUC__)
#	ifdef __clang__
#		define DPP_USE_CLANG
#	else
#		define DPP_USE_GCC
#	endif
#else
#	error "Unknown compiler"
#endif

#ifdef __cplusplus
#	define DPP_USE_CPP
#	if defined(DPP_USE_WINDOWS) && defined(_MSC_VER)
#		define DPP_USE_CPP_VERSION _MSVC_LANG
#	else
#		define DPP_USE_CPP_VERSION __cplusplus
#	endif
#elif defined(__STDC__)
#	define DPP_USE_C 1
#	define DPP_USE_C_VERSION __STDC_VERSION__
#else
#	error "Unknown language"
#endif

#ifdef __MINGW32__
#	define DPP_USE_MINGW
#endif