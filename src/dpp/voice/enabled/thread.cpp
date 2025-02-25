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

#include <dpp/exception.h>
#include <dpp/isa_detection.h>
#include <dpp/discordvoiceclient.h>
#include "../../dave/encryptor.h"
#include "enabled.h"

namespace dpp {

void discord_voice_client::on_disconnect() {

	time_t current_time = time(nullptr);

	/* Here, we check if it's been longer than 3 seconds since the previous loop,
	 * this gives us time to see if it's an actual disconnect, or an error.
	 * This will prevent us from looping too much, meaning error codes do not cause an infinite loop.
	 */
	if (current_time - last_loop_time >= 3) {
		times_looped = 0;
	}

	/* This does mean we'll always have times_looped at a minimum of 1, this is intended. */
	times_looped++;

	/* If we've looped 5 or more times, abort the loop. */
	if (terminating || times_looped >= 5) {
		log(dpp::ll_warning, "Reached max loops whilst attempting to read from the websocket. Aborting websocket.");
		return;
	}
	last_loop_time = current_time;

	ssl_connection::close();
	owner->start_timer([this](auto handle) {
		log(dpp::ll_debug, "Attempting to reconnect voice websocket " + std::to_string(channel_id) + " to wss://" + hostname + "...");
		owner->stop_timer(handle);
		cleanup();
		if (timer_handle) {
			owner->stop_timer(timer_handle);
			timer_handle = 0;
		}
		start = time(nullptr);
		setup();
		terminating = false;
		ssl_connection::connect();
		websocket_client::connect();
		run();
	}, 1);
}

void discord_voice_client::run() {
	ssl_connection::read_loop();
}

}