/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <string_view>
#include <dpp/exception.h>
#include <dpp/isa_detection.h>
#include <dpp/discordvoiceclient.h>
#include "../../dave/encryptor.h"
#include "enabled.h"

namespace dpp {

void discord_voice_client::thread_run()
{
	utility::set_thread_name(std::string("vc/") + std::to_string(server_id));

	size_t times_looped = 0;
	time_t last_loop_time = time(nullptr);

	do {
		bool error = false;
		ssl_client::read_loop();
		ssl_client::close();

		time_t current_time = time(nullptr);
		/* Here, we check if it's been longer than 3 seconds since the previous loop,
		 * this gives us time to see if it's an actual disconnect, or an error.
		 * This will prevent us from looping too much, meaning error codes do not cause an infinite loop.
		 */
		if (current_time - last_loop_time >= 3)
			times_looped = 0;

		/* This does mean we'll always have times_looped at a minimum of 1, this is intended. */
		times_looped++;
		/* If we've looped 5 or more times, abort the loop. */
		if (times_looped >= 5) {
			log(dpp::ll_warning, "Reached max loops whilst attempting to read from the websocket. Aborting websocket.");
			break;
		}

		last_loop_time = current_time;

		if (!terminating) {
			log(dpp::ll_debug, "Attempting to reconnect the websocket...");
			do {
				try {
					ssl_client::connect();
					websocket_client::connect();
				}
				catch (const std::exception &e) {
					log(dpp::ll_error, std::string("Error establishing voice websocket connection, retry in 5 seconds: ") + e.what());
					ssl_client::close();
					std::this_thread::sleep_for(std::chrono::seconds(5));
					error = true;
				}
			} while (error && !terminating);
		}
	} while(!terminating);
}

void discord_voice_client::run()
{
	this->runner = new std::thread(&discord_voice_client::thread_run, this);
	this->thread_id = runner->native_handle();
}


}
