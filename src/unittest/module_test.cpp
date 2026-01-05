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

#ifdef DPP_MODULES
#include "test.h"

import dpp;

/**
 * @brief Test basic dpp types and functionality via module import
 */
void test_dpp_module_basic() {
	start_test(MODULE_IMPORT_BASIC);

	dpp::snowflake test_id = 825411104208977952ULL;
	time_t extracted_timestamp = test_id.get_creation_time();
	if (extracted_timestamp != 1616978723) {
		set_status(MODULE_IMPORT_BASIC, ts_failed, "snowflake timestamp extraction");
		return;
	}

	// User object creation test
	dpp::user test_user;
	test_user.id = 987654321;
	test_user.username = "ModuleTestUser";
	if (test_user.id != 987654321 || test_user.username != "ModuleTestUser") {
		set_status(MODULE_IMPORT_BASIC, ts_failed, "user object creation");
		return;
	}

	// Testing ts_to_string
	std::string test_time = dpp::ts_to_string(1642611864);
	if (test_time != "2022-01-19T17:04:24Z") {
		set_status(MODULE_IMPORT_BASIC, ts_failed, "timestamp conversion");
		return;
	}

	// Message object creation test
	dpp::message test_msg;
	test_msg.content = "Test message from module";
	test_msg.id = 111222333;

	auto is_valid_message = [](const dpp::message& msg, std::string_view s, int test_id) -> bool {
		return msg.content == s && msg.id == test_id;
	};

	if (!is_valid_message(test_msg, "Test message from module")) {
		set_status(MODULE_IMPORT_BASIC, ts_failed, "message object creation");
		return;
	}

	// URL encoding test
	std::string encoded = dpp::utility::url_encode("test value");
	if (encoded != "test%20value") {
		set_status(MODULE_IMPORT_BASIC, ts_failed, "URL encoding");
		return;
	}

	// Markdown escaping test
	std::string markdown = "**bold** _italic_";
	std::string escaped = dpp::utility::markdown_escape(markdown);
	if (escaped != "\\*\\*bold\\*\\* \\_italic\\_") {
		set_status(MODULE_IMPORT_BASIC, ts_failed, "markdown escaping");
		return;
	}

	// Role comparison test
	dpp::role r1;
	dpp::role r2;
	r1.id = 100;
	r1.position = 1;
	r1.guild_id = 500;
	r2.id = 200;
	r2.position = 2;
	r2.guild_id = 500;

	auto role_comparison_1 = [](const dpp::role& r1, const dpp::role& r2) -> bool {
		return (r1 < r2);
	};

	auto role_comparison_2 = [](const dpp::role& r1, const dpp::role& r2) -> bool {
		return !(r1 > r2);
	}

	if (!role_comparison_1(r1, r2) || !role_comparison_2(r1, r2)) {
		set_status(MODULE_IMPORT_BASIC, ts_failed, "role comparison");
		return;
	}

	set_status(MODULE_IMPORT_BASIC, ts_success);
}

/**
 * @brief Test dpp module coroutine support (if enabled)
 */
void test_dpp_module_coro() {
	start_test(MODULE_IMPORT_CORO);

#ifndef DPP_NO_CORO
	dpp::promise<int> test_promise;
	test_promise.set_value(42);

	if constexpr (!requires { dpp::task<void>{}; }) {
		set_status(MODULE_IMPORT_CORO, ts_failed, "coroutine types not accessible");
		return;
	}

	set_status(MODULE_IMPORT_CORO, ts_success);
#else
	set_status(MODULE_IMPORT_CORO, ts_skipped, "coroutines disabled");
#endif
}

/**
 * @brief Main entry point for module tests
 */
void run_module_tests() {
	test_dpp_module_basic();
	test_dpp_module_coro();
}

#endif
