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

#include <dpp/dpp.h>
#include <iostream>

int main() {
	char* t = getenv("DPP_UNIT_TEST_TOKEN");
	if (t) {
		dpp::cluster soak_test(t);
		soak_test.set_websocket_protocol(dpp::ws_etf);
		soak_test.on_log(dpp::utility::cout_logger());
		soak_test.start(dpp::st_return);
		while (true) {
			sleep(60);
			dpp::discord_client* dc = soak_test.get_shard(0);
			if (dc != nullptr) {
				std::cout << "Websocket latency: " << std::fixed << dc->websocket_ping << " Guilds: " << dpp::get_guild_count() << " Users: " << dpp::get_user_count() << "\n";
			}
		}
	}
}
