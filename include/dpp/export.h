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

/* Compile-time assertion check for C++17 */
// Investigate: MSVC doesn't like this
//static_assert(__cplusplus >= 201703L, "D++ Requires a C++17 compatible compiler. Please ensure that you have enabled C++17 in your compiler flags.");

#ifdef DPP_BUILD

	#ifdef _WIN32
		#include <dpp/win32_safe_warnings.h>
	#endif

	#ifdef _WIN32
		#define DPP_EXPORT __declspec(dllexport)
	#else
		#define DPP_EXPORT
	#endif
#else
	#ifdef _WIN32
		#define DPP_EXPORT __declspec(dllimport)
		/* This is required otherwise fmt::format requires additional file linkage to your project */
		#define FMT_HEADER_ONLY
	#else
		#define DPP_EXPORT
	#endif
#endif

#ifndef _WIN32
	#define SOCKET int
#else
	#include <WinSock2.h>
#endif
