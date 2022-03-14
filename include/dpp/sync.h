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
#pragma once
#include <dpp/export.h>
#include <dpp/snowflake.h>
#include <chrono>

namespace dpp {

/*
 * DPP_VA_COMMA() expands to nothing if given no arguments and a comma if
 * given 1 to 8 arguments.  Bad things happen if given more than 8
 * arguments.  Don't do it.
 */
#define DPP_VA_COMMA(...) DPP_GET_LAST_ARG(,##__VA_ARGS__,DPP_COMMA,DPP_COMMA,DPP_COMMA,DPP_COMMA,DPP_COMMA,DPP_COMMA,DPP_COMMA,DPP_COMMA,)
#define DPP_GET_LAST_ARG(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,...) a10
#define DPP_COMMA ,

/**
 * @brief Call a D++ REST function synchronously.
 * Synchronously calling a REST function means IT WILL BLOCK. This is a Bad Thing™️ and strongly discouraged.
 * There are very few circumstances you actually need this. If you do need to use this, you'll know it.
 * @param type The type of value to be returned from the REST call
 * @param cluster A pointer to a dpp::cluster object
 * @param func The function in dpp::cluster to call
 * @param ... Any number of required parameters to the function, minus the callback
 * @return The returned data type
 * @throw dpp::rest_exception Will throw on failure to complete the request.
 * Exceptions are thrown in the thread which called dpp_sync().
 */
#define dpp_sync(type, cluster, func, ...) \
	([cluster]() -> type { \
		bool completed = false; \
		bool except = false; \
		std::string message; \
		type _t = {}; \
 		(cluster)->func( __VA_ARGS__ DPP_VA_COMMA(__VA_ARGS__) [&except, &message, &_t, &completed](const auto& cc) { \
		 	if (cc.is_error()) { \
				message = cc.get_error().message; \
			 	except = true; \
			} else { \
				try { \
					_t = std::get<type>(cc.value); \
				} \
				catch (const std::exception& e) { \
					message = e.what(); \
					except = true; \
				} \
			} \
			completed = true; \
		}); \
		do { \
			std::this_thread::sleep_for(std::chrono::microseconds(10)); \
		} while (!completed); \
		if (except) { \
			throw dpp::rest_exception(message); \
		} \
		return _t; \
	})()

};
