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
	if (t == nullptr || getenv("TEST_GUILD_ID") == nullptr || getenv("TEST_VC_ID") == nullptr) {
		std::cerr << "Missing unit test environment. Set DPP_UNIT_TEST_TOKEN, TEST_GUILD_ID, and TEST_VC_ID\n";
		exit(1);
	}
	dpp::snowflake TEST_GUILD_ID(std::string(getenv("TEST_GUILD_ID")));
	dpp::snowflake TEST_VC_ID(std::string(getenv("TEST_VC_ID")));
	std::cout << "Test Guild ID: " << TEST_GUILD_ID << " Test VC ID: " << TEST_VC_ID << "\n\n";
	dpp::cluster dave_test(t, dpp::i_default_intents, 1, 0, 1, false, dpp::cache_policy_t{ dpp::cp_none, dpp::cp_none, dpp::cp_none, dpp::cp_none, dpp::cp_none });

	dave_test.on_log([&](const dpp::log_t& log) {
		std::cout << "[" << dpp::utility::current_date_time() << "] " << dpp::utility::loglevel(log.severity) << ": " << log.message << std::endl;
	});

	dave_test.on_guild_create([&](const dpp::guild_create_t & event) {
		if (event.created->id == TEST_GUILD_ID) {
			dpp::discord_client* s = dave_test.get_shard(0);
			s->connect_voice(TEST_GUILD_ID, TEST_VC_ID, false, false, true);
		}
	});
	dave_test.start(false);
}
