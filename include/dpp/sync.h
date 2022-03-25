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

/**
 * @brief Call a D++ REST function synchronously.
 * 
 * Synchronously calling a REST function means *IT WILL BLOCK* - This is a Bad Thing™ and strongly discouraged.
 * There are very few circumstances you actually need this. If you do need to use this, you'll know it.
 * 
 * Example:
 * 
 * ```cpp
 * dpp::message m = dpp::sync<dpp::message>(&bot, &dpp::cluster::message_create, dpp::message(channel_id, "moo."));
 * ```
 * 
 * @warning As previously mentioned, this template will block. It is ill-advised to call this outside of
 * a separate thread and this should never be directly used in any event such as dpp::cluster::on_interaction_create!
 * @tparam T type of expected return value, should match up with the method called
 * @tparam F Type of class method in dpp::cluster to call.
 * @tparam Ts Function parameters in method call
 * @param c A pointer to dpp::cluster object
 * @param func pointer to class method in dpp::cluster to call. This can call any
 * dpp::cluster member function who's last parameter is a dpp::command_completion_event_t callback type.
 * @param args Zero or more arguments for the method call
 * @return An instantiated object of type T
 * @throw dpp::rest_exception On failure of the method call, an exception is thrown
 */
template<typename T, class F, class... Ts> T sync(class cluster* c, F func, Ts... args) {
	bool except = false;
	std::string message;

    std::mutex sync_mutex;
    std::unique_lock<std::mutex> sync_guard(sync_mutex);
    std::condition_variable sync;

	/* Passing _t into the lambda is SAFE here, as this function is 
	 * guaranteed to stick around until execution of the REST call is finished.
	 */
	T _t = {};
	/* (obj ->* func) is the obscure syntax for calling a method pointer on an object instance */
	(c ->* func)(args..., [&sync, &except, &message, &_t](const auto& cc) {
		if (cc.is_error()) {
			message = cc.get_error().message;
			except = true;
		} else {
			try {
				_t = std::get<T>(cc.value);
			}
			catch (const std::exception& e) {
				/* As we are inside a separate thread here, we can't just
				 * let this exception loose into the stack. It'll be thrown
				 * in a place where the user has no means to catch it. Instead
				 * record the failure, and re-throw later from the calling
				 * thread.
				 */
				message = e.what();
				except = true;
			}
		}

        // unblock calling thread
        sync.notify_all();
	});

    /* blocking this thread until rest request
     * is finished
     */
    sync.wait(sync_guard);

	if (except) {
		/* Re-throw any exceptions encountered on the other thread */
		throw dpp::rest_exception(message);
	}
	return _t;
}

};