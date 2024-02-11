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

#include <dpp/restrequest.h>

 /* Unit tests for Cache */
void cache_tests(dpp::cluster& bot) {
	set_test(USER_GET_CACHED_PRESENT, false);
	try {
		dpp::user_identified u = bot.user_get_cached_sync(TEST_USER_ID);
		set_test(USER_GET_CACHED_PRESENT, (u.id == TEST_USER_ID));
	}
	catch (const std::exception&) {
		set_test(USER_GET_CACHED_PRESENT, false);
	}

	set_test(USER_GET_CACHED_ABSENT, false);
	try {
		/* This is the snowflake ID of a discord staff member.
		 * We assume here that staffer's discord IDs will remain constant
		 * for long periods of time and they won't lurk in the unit test server.
		 * If this becomes not true any more, we'll pick another well known
		 * user ID.
		 */
		dpp::user_identified u = bot.user_get_cached_sync(90339695967350784);
		set_test(USER_GET_CACHED_ABSENT, (u.id == dpp::snowflake(90339695967350784)));
	}
	catch (const std::exception&) {
		set_test(USER_GET_CACHED_ABSENT, false);
	}

	set_test(CUSTOMCACHE, false);
	dpp::cache<test_cached_object_t> testcache;
	test_cached_object_t* tco = new test_cached_object_t(666);
	tco->foo = "bar";
	testcache.store(tco);
	test_cached_object_t* found_tco = testcache.find(666);
	if (found_tco && found_tco->id == dpp::snowflake(666) && found_tco->foo == "bar") {
		set_test(CUSTOMCACHE, true);
	}
	else {
		set_test(CUSTOMCACHE, false);
	}
	testcache.remove(found_tco);
}