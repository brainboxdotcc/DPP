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

#include <dpp/dpp.h>
#include <iostream>
#include <thread>
#include <atomic>
#ifndef _WIN32
	#include <csignal>
#endif

dpp::cluster* s{nullptr};
std::atomic_bool signalled{false};

int main() {
	using namespace std::chrono_literals;
	char* t = getenv("DPP_UNIT_TEST_TOKEN");
	if (t) {
		dpp::cluster soak_test(t, dpp::i_default_intents | dpp::i_guild_members, 1, 0, 1);
		s = &soak_test;
		//soak_test.set_websocket_protocol(dpp::ws_etf);
		soak_test.on_log([&](const dpp::log_t& log) {
			std::cout << "[" << dpp::utility::current_date_time() << "] " << dpp::utility::loglevel(log.severity) << ": " << log.message << std::endl;
		});
		soak_test.start(dpp::st_return);

#ifndef _WIN32
		signal(SIGUSR1, [](int sig) {
			signalled = true;
		});
#endif

		while (true) {
			std::this_thread::sleep_for(1s);
			dpp::discord_client* dc = soak_test.get_shard(0);
			if (dc != nullptr) {
				if (time(nullptr) % 60 == 0) {
					std::cout << "Websocket latency: " << std::fixed << dc->websocket_ping << " Guilds: " << dpp::get_guild_count() << " Users: " << dpp::get_user_count() << "\n";
				}
				if (signalled) {
					signalled = false;
					dc->close();
				}
			}
		}
	}
}
