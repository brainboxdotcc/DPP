/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2022 Craig Edwards and D++ contributors
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

#ifdef DPP_CORO
#pragma once

#if (defined(_LIBCPP_VERSION) and !defined(__cpp_impl_coroutine)) // if libc++ experimental implementation (LLVM < 14)
#  define STDCORO_EXPERIMENTAL_HEADER
#  define STDCORO_EXPERIMENTAL_NAMESPACE
#endif

#ifdef STDCORO_GLIBCXX_COMPAT
#  define __cpp_impl_coroutine 1
namespace std {
	namespace experimental {
		using namespace std;
	}
}
#endif

#ifdef STDCORO_EXPERIMENTAL_HEADER
#  include <experimental/coroutine>
#else
#  include <coroutine>
#endif

namespace dpp {

/**
	* @brief Implementation details for internal use only.
	*
	* @attention This is only meant to be used by D++ internally. Support will not be given regarding the facilities in this namespace.
	*/
namespace detail {
#ifdef _DOXYGEN_
/**
	* @brief Alias for either std or std::experimental depending on compiler and library. Used by coroutine implementation.
	*
	* @todo Remove and use std when all supported libraries have coroutines in it
	*/
namespace std_coroutine {}
#else
#  ifdef STDCORO_EXPERIMENTAL_NAMESPACE
namespace std_coroutine = std::experimental;
#  else
namespace std_coroutine = std;
#  endif
#endif

} // namespace detail

struct confirmation_callback_t;

template <typename R = confirmation_callback_t>
class async;

template <typename R = void>
#ifndef _DOXYGEN_
requires std::is_same_v<void, R> || (!std::is_reference_v<R> && std::is_move_constructible_v<R> && std::is_move_assignable_v<R>)
#endif
class task;

struct job;

} // namespace dpp

#endif /* DPP_CORO */

