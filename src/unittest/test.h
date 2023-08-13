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
#undef DPP_BUILD
#ifdef _WIN32
_Pragma("warning( disable : 4251 )"); // 4251 warns when we export classes or structures with stl member variables
_Pragma("warning( disable : 5105 )"); // 4251 warns when we export classes or structures with stl member variables
#endif
#include <dpp/dpp.h>
#include <dpp/json_fwd.h>
#include <iomanip>

#ifdef _WIN32
#define SHARED_OBJECT "dpp.dll"
#else
#define SHARED_OBJECT "libdpp.so"
#endif

using json = nlohmann::json;

enum test_type_t {
	/* A test that does not require discord connectivity */
	tt_offline,
	/* A test that requires discord connectivity */
	tt_online,
	/* A test that requires both online and full tests to be enabled */
	tt_extended
};

/* Represents a test case */
struct test_t {
	/* Test type */
	test_type_t type;
	/* Description of test */
	std::string description;
	/* Has been executed */
	bool executed = false;
	/* Was successfully tested */
	bool success = false;
};

class test_cached_object_t : public dpp::managed {
public:
	test_cached_object_t(dpp::snowflake _id) : dpp::managed(_id) { };
	virtual ~test_cached_object_t() = default;
	std::string foo;
};

/* How long the unit tests can run for */
const int64_t TEST_TIMEOUT = 60;

#define SAFE_GETENV(var) (getenv(var) && *(getenv(var)) ? getenv(var) : "0")

/* IDs of various channels and guilds used to test */
extern dpp::snowflake TEST_GUILD_ID;
extern dpp::snowflake TEST_TEXT_CHANNEL_ID;
extern dpp::snowflake TEST_VC_ID;
extern dpp::snowflake TEST_USER_ID;
extern dpp::snowflake TEST_EVENT_ID;

/* True if we skip tt_online tests */
extern bool offline;
extern bool extended;

/**
 * @brief Perform a test of a REST base API call with one parameter
 */
#define singleparam_api_test(func_name, param, return_type, testname) \
	set_test(testname, false); \
	if (!offline) { \
		bot.func_name (param, [&](const dpp::confirmation_callback_t &cc) { \
			if (!cc.is_error()) { \
				return_type g = std::get<return_type>(cc.value); \
				if (g.id == param) { \
					set_test(testname, true); \
				} else { \
					bot.log(dpp::ll_debug, cc.http_info.body); \
					set_test(testname, false); \
				} \
			} else { \
				bot.log(dpp::ll_debug, cc.http_info.body); \
				set_test(testname, false); \
			} \
		}); \
	}

/**
 * @brief Perform a test of a REST base API call with one parameter
 */
#define twoparam_api_test(func_name, param1, param2, return_type, testname) \
	set_test(testname, false); \
	if (!offline) { \
		bot.func_name (param1, param2, [&](const dpp::confirmation_callback_t &cc) { \
			if (!cc.is_error()) { \
				return_type g = std::get<return_type>(cc.value); \
				if (g.id > 0) { \
					set_test(testname, true); \
				} else { \
					bot.log(dpp::ll_debug, cc.http_info.body); \
					set_test(testname, false); \
				} \
			} else { \
				bot.log(dpp::ll_debug, cc.http_info.body); \
				set_test(testname, false); \
			} \
		}); \
	}

/**
 * @brief Perform a test of a REST base API call with one parameter that returns a list
 */
#define singleparam_api_test_list(func_name, param, return_type, testname) \
	set_test(testname, false); \
	if (!offline) { \
		bot.func_name (param, [&](const dpp::confirmation_callback_t &cc) { \
			if (!cc.is_error()) { \
				return_type g = std::get<return_type>(cc.value); \
				if (g.size() > 0) { \
					set_test(testname, true); \
				} else { \
					set_test(testname, false); \
					bot.log(dpp::ll_debug, cc.http_info.body); \
				} \
			} else { \
				set_test(testname, false); \
				bot.log(dpp::ll_debug, cc.http_info.body); \
			} \
		}); \
	}

/**
 * @brief Perform a test of a REST base API call with one parameter that returns a list
 */
#define multiparam_api_test_list(func_name, param, return_type, testname) \
	set_test(testname, false); \
	if (!offline) { \
		bot.func_name (param, 0, 0, 1000, [&](const dpp::confirmation_callback_t &cc) { \
			if (!cc.is_error()) { \
				return_type g = std::get<return_type>(cc.value); \
				if (g.size() > 0) { \
					set_test(testname, true); \
				} else { \
					set_test(testname, false); \
					bot.log(dpp::ll_debug, cc.http_info.body); \
				} \
			} else { \
				set_test(testname, false); \
				bot.log(dpp::ll_debug, cc.http_info.body); \
			} \
		}); \
	}

/**
 * @brief Perform a test of a REST base API call with two parameters
 */
#define twoparam_api_test_list(func_name, param1, param2, return_type, testname) \
	set_test(testname, false); \
	if (!offline) { \
		bot.func_name (param1, param2, [&](const dpp::confirmation_callback_t &cc) { \
			if (!cc.is_error()) { \
				return_type g = std::get<return_type>(cc.value); \
				if (g.size() > 0) { \
					set_test(testname, true); \
				} else { \
					bot.log(dpp::ll_debug, cc.http_info.body); \
					set_test(testname, false); \
				} \
			} else { \
				bot.log(dpp::ll_debug, cc.http_info.body); \
				set_test(testname, false); \
			} \
		}); \
	}


/**
 * @brief Perform a test of a REST base API call with no parameters
 */
#define noparam_api_test(func_name, return_type, testname) \
	set_test(testname, false); \
	if (!offline) { \
		bot.func_name ([&](const dpp::confirmation_callback_t &cc) { \
			if (!cc.is_error()) { \
				return_type g = std::get<return_type>(cc.value); \
				set_test(testname, true); \
			} else { \
				bot.log(dpp::ll_debug, cc.http_info.body); \
				set_test(testname, false); \
			} \
		}); \
	}

/**
 * @brief Sets a test's status
 * 
 * @param testname test name (key) to set the status of
 * @param success If set to true, sets success to true, if set to false and called
 * once, sets executed to true, if called twice, also sets success to false.
 * This means that before you run the test you should call this function once
 * with success set to false, then if/wen the test completes call it again with true.
 * If the test fails, call it a second time with false, or not at all.
 */
void set_test(const std::string &testname, bool success = false);

/**
 * @brief Prints a summary of all tests executed
 * @param tests List of tests executed
 * 
 * @return int Returns number of failed tests, for use as a return value from the main() function
 */
int test_summary();


/**
 * @brief Load test audio for the voice channel tests
 * 
 * @return std::vector<uint8_t> data and size for test audio
 */
std::vector<uint8_t> load_test_audio();

/**
 * @brief Load test image for the attachment tests
 * 
 * @return std::vector<uint8_t> data and size for test image
 */
std::vector<uint8_t> load_test_image();

/**
 * @brief Get the token from the environment variable DPP_UNIT_TEST_TOKEN
 * 
 * @return std::string token
 * @note If the environment variable does not exist, this will exit the program.
 */
std::string get_token();

/**
 * @brief Wait for all tests to be completed or test program to time out
 */
void wait_for_tests();

/**
 * @brief Get the start time of tests
 * 
 * @return double start time in fractional seconds
 */
double get_start_time();

/**
 * @brief Get the current execution time in seconds
 * 
 * @return double fractional seconds
 */
double get_time();

/**
 * @brief A test version of the message collector for use in unit tests
 */
class message_collector : public dpp::message_collector {
public:
	message_collector(dpp::cluster* cl, uint64_t duration) : dpp::message_collector(cl, duration) { }

	virtual void completed(const std::vector<dpp::message>& list) {
		set_test("MSGCOLLECT", list.size() > 0);
	}
};

/**
 * @brief Convenience functor to get the snowflake of a certain type
 */
struct user_project_id_t {
	dpp::snowflake operator()(const dpp::user &user) const noexcept {
		return user.id;
	}

	dpp::snowflake operator()(const dpp::guild_member &user) const noexcept {
		return user.user_id;
	}

	dpp::snowflake operator()(dpp::snowflake user) const noexcept {
		return user;
	}

	dpp::snowflake operator()(const dpp::thread_member &user) const noexcept {
		return user.user_id;
	}
};

/**
 * @brief Convenience lambda to get the user snowflake of a certain user type
 * @see user_project_id_t
 */
inline constexpr user_project_id_t get_user_snowflake;

/**
 * @brief Convenience lambda to check if a certain user is the owner of the test bot, mostly meant to be passed to standard algorithms
 * @see get_user_snowflake
 *
 * @return bool whether the user is the test bot owner
 */
inline constexpr auto is_owner = [](auto &&user) noexcept {
	return get_user_snowflake(user) == TEST_USER_ID;
};
