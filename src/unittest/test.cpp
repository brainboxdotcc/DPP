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
#include "test.h"

/* Unit tests go here */
int main(int argc, char *argv[])
{
	std::string token(get_token());

	std::cout << "[" << std::fixed << std::setprecision(3) << (dpp::utility::time_f() - get_start_time()) << "]: [\u001b[36mSTART\u001b[0m] ";
	if (offline) {
		std::cout << "Running offline unit tests only.\n";
	} else {
		if (argc > 1 && std::find_if(argv + 1, argv + argc, [](const char *a){ return (std::strcmp(a, "full") == 0); }) != argv + argc) {
			extended = true;
		}
		std::cout << "Running offline and " << (extended ? "extended" : "limited") << " online unit tests. Guild ID: " << TEST_GUILD_ID << " Text Channel ID: " << TEST_TEXT_CHANNEL_ID << " VC ID: " << TEST_VC_ID << " User ID: " << TEST_USER_ID << " Event ID: " << TEST_EVENT_ID << "\n";
	}

	dpp::cluster bot(token, dpp::i_all_intents);

	errors_test();
	http_client_tests(token);
	discord_objects_tests();
	gateway_events_tests(token, bot);
	cache_tests(bot);
	utility_tests();

	{
		coro_offline_tests();
	}

	/* Return value = number of failed tests, exit code 0 = success */
	return test_summary();
}
