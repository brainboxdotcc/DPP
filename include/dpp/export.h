/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2021 Craig Edwards and D++ contributors
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/
#pragma once

/* Compile-time check for C++17.
 * Either one of the following causes a compile time error:
 * __cplusplus not defined at all (this means we are being compiled on a C compiler)
 * MSVC defined and _MSVC_LANG < 201703L (Visual Studio, but not C++17 or newer)
 * MSVC not defined and __cplusplus < 201703L (Non-visual studio, but not C++17 or newer)
 * The additional checks are required because MSVC doesn't correctly set __cplusplus to 201703L,
 * which is hugely non-standard, but apparently "it broke stuff" so they dont ever change it
 * from C++98. Ugh.
 */
#if (!defined(__cplusplus) || (defined(_MSC_VER) && (!defined(_MSVC_LANG) || _MSVC_LANG < 201703L)) || (!defined(_MSC_VER) && __cplusplus < 201703L))
	#error "D++ Requires a C++17 compatible C++ compiler. Please ensure that you have enabled C++17 in your compiler flags."
#endif

#ifndef DPP_STATIC
	/* Dynamic linked build as shared object or dll */
	#ifdef DPP_BUILD
		/* Building the library */
		#ifdef _WIN32
			#include <dpp/win32_safe_warnings.h>
			#define DPP_EXPORT __declspec(dllexport)
		#else
			#define DPP_EXPORT
		#endif
	#else
		/* Including the library */
		#ifdef _WIN32
			#define DPP_EXPORT __declspec(dllimport)
		#else
			#define DPP_EXPORT
		#endif
	#endif
#else
	/* Static linked build */
	#if defined(_WIN32) && defined(DPP_BUILD)
		#include <dpp/win32_safe_warnings.h>
	#endif
	#define DPP_EXPORT
#endif

#ifndef _WIN32
	#define SOCKET int
#else
	#include <WinSock2.h>
#endif