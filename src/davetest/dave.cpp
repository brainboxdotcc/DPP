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
#include <string>

int main() {
	using namespace std::chrono_literals;
	char* t = getenv("DPP_UNIT_TEST_TOKEN");
	dpp::snowflake TEST_GUILD_ID(std::string(getenv("TEST_GUILD_ID")));
	dpp::snowflake TEST_VC_ID(std::string(getenv("TEST_VC_ID")));
	if (t) {
		dpp::cluster dave_test(t, dpp::i_default_intents | dpp::i_guild_members);
		dave_test.set_websocket_protocol(dpp::ws_etf);
		dave_test.on_log(dpp::utility::cout_logger());

		dave_test.on_guild_create([&](const dpp::guild_create_t & event) {
			if (event.created->id == TEST_GUILD_ID) {
				dpp::discord_client* s = dave_test.get_shard(0);
				s->connect_voice(TEST_GUILD_ID, TEST_VC_ID, false, false, true);
			}
		});
		dave_test.start(dpp::st_wait);
	}
}
