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
#include "test.h"

/* Current list of unit tests */
std::map<std::string, test_t> tests = {
	{"CLUSTER", {"Instantiate DPP cluster", false, false}},
	{"BOTSTART", {"cluster::start method", false, false}},
	{"CONNECTION", {"Connection to client websocket", false, false}},
	{"APPCOMMAND", {"Creation of application command", false, false}},
	{"DELCOMMAND", {"Deletion of application command", false, false}},
	{"LOGGER", {"Log events", false, false}},
	{"MESSAGECREATE", {"Creation of a channel message", false, false}},
	{"MESSAGEDELETE", {"Deletion of a channel message", false, false}},
	{"MESSAGERECEIVE", {"Receipt of a created message", false, false}},
	{"CACHE", {"Test guild cache", false, false}},
	{"USERCACHE", {"Test user cache", false, false}},
	{"VOICECONN", {"Connect to voice channel", false, false}},
	{"VOICESEND", {"Send audio to voice channel", false, false}},
	{"REACT", {"React to a message", false, false}},
	{"REACTEVENT", {"Reaction event", false, false}},
	{"GUILDCREATE", {"Receive guild create event", false, false}},
	{"MESSAGESGET", {"Get messages", false, false}},
	{"TIMESTAMP", {"crossplatform_strptime()", false, false}},
	{"ICONHASH", {"utility::iconhash", false, false}},
	{"CURRENTUSER", {"cluster::current_user_get()", false, false}},
	{"GETGUILD", {"cluster::guild_get()", false, false}},
	{"GETCHAN", {"cluster::channel_get()", false, false}},
	{"GETCHANS", {"cluster::channels_get()", false, false}},
	{"GETROLES", {"cluster::roles_get()", false, false}},
	{"GETINVS", {"cluster::guild_get_invites()", false, false}},
	{"GETBANS", {"cluster::guild_get_bans()", false, false}},
	{"GETPINS", {"cluster::channel_pins_get()", false, false}},
	{"GETEVENTS", {"cluster::guild_events_get()", false, false}},
	{"GETEVENT", {"cluster::guild_event_get()", false, false}},
	{"MSGMENTIONUSER", {"message_create_t::reply() (mention)", false, false}},
	{"MSGCREATESEND", {"message_create_t::send()", false, false}},
	{"MSGCREATEREPLY", {"message_create_t::reply()", false, false}},
	{"GETEVENTUSERS", {"cluster::guild_event_users_get()", false, false}},
	{"TIMERSTART", {"start timer", false, false}},
	{"TIMERSTOP", {"stop timer", false, false}},
	{"ONESHOT", {"one-shot timer", false, false}},
	{"PRESENCE", {"Presence intent", false, false}},
	{"CUSTOMCACHE", {"Instantiate a cache", false, false}},
	{"MSGCOLLECT", {"message_collector", false, false}},
	{"TS", {"managed::get_creation_date()", false, false}},
	{"READFILE", {"utility::read_file()", false, false}},
	{"TIMESTAMPTOSTRING", {"dpp::ts_to_string()", false, false}},
	{"TIMESTRINGTOTIMESTAMP", {"dpp::ts_not_null()", false, false}},
	{"COMMANDOPTIONCHOICEFILLFROMJSON", {"dpp::command_option_choice::fill_from_json()", false, false}},
	{"HOSTINFO", {"dpp::https_client::get_host_info()", false, false}},
	{"HTTPS", {"dpp::https_client HTTPS request", false, false}},
	{"HTTP", {"dpp::https_client HTTP request", false, false}},
	{"RUNONCE", {"dpp::run_once<T>", false, false}},
	{"WEBHOOK", {"dpp::webhook construct from URL", false, false}},
	{"MD_ESC_1", {"Markdown escaping (ignore code block contents)", false, false}},
	{"MD_ESC_2", {"Markdown escaping (escape code block contents)", false, false}},
};

double start = dpp::utility::time_f();

dpp::snowflake TEST_GUILD_ID = std::stoull(SAFE_GETENV("TEST_GUILD_ID"));
dpp::snowflake TEST_TEXT_CHANNEL_ID = std::stoull(SAFE_GETENV("TEST_TEXT_CHANNEL_ID"));
dpp::snowflake TEST_VC_ID = std::stoull(SAFE_GETENV("TEST_VC_ID"));
dpp::snowflake TEST_USER_ID = std::stoull(SAFE_GETENV("TEST_USER_ID"));
dpp::snowflake TEST_EVENT_ID = std::stoull(SAFE_GETENV("TEST_EVENT_ID"));

void set_test(const std::string &testname, bool success) {
	auto i = tests.find(testname);
	if (i != tests.end()) {
		if (!i->second.executed) {
			std::cout << "[" << std::fixed << std::setprecision(3)  << get_time() << "]: " << "[\u001b[33mTESTING\u001b[0m] " << i->second.description << "\n";
		} else if (!success) {
			std::cout << "[" << std::fixed << std::setprecision(3) << get_time() << "]: " << "[\u001b[31mFAILED\u001b[0m] " << i->second.description << "\n";
		}
		i->second.executed = true;
		if (success) {
			i->second.success = true;
			std::cout << "[" << std::fixed << std::setprecision(3) << get_time() << "]: " << "[\u001b[32mSUCCESS\u001b[0m] " << i->second.description << "\n";
		}
	}
}

double get_start_time() {
	return start;
}

double get_time() {
	return dpp::utility::time_f() - get_start_time();
}

int test_summary() {
	/* Report on all test cases */
	int failed = 0, passed = 0;
	std::cout << "\u001b[37;1m\n\nUNIT TEST SUMMARY\n==================\n\u001b[0m";
	for (auto & t : tests) {
		if (t.second.success == false || t.second.executed == false) {
			failed++;
		} else {
			passed++;
		}
		std::cout << std::left << std::setw(50) << t.second.description << " " << std::fixed << std::setw(6) << (t.second.executed && t.second.success ? "\u001b[32mPASS" : "\u001b[31mFAIL") << std::setw(0) << "\u001b[0m\n";
	}
	std::cout << "\u001b[37;1m\nExecution finished in " << std::fixed << std::setprecision(3) <<  get_time() << std::setprecision(0) << " seconds.\nFailed: " << failed << " Passed: " << passed << " Percentage: " << std::setprecision(2) << ((float)(passed) / (float)(tests.size()) * 100.0f) << "%\u001b[0m\n";
	return failed;
}

std::vector<uint8_t> load_test_audio() {
	std::vector<uint8_t> testaudio;
	std::ifstream input ("../testdata/Robot.pcm", std::ios::in|std::ios::binary|std::ios::ate);
	if (input.is_open()) {
		size_t testaudio_size = input.tellg();
		testaudio.resize(testaudio_size);
		input.seekg(0, std::ios::beg);
		input.read((char*)testaudio.data(), testaudio_size);
		input.close();
	}
	else {
		std::cout << "ERROR: Can't load ../testdata/Robot.pcm\n";
		exit(1);
	}
	return testaudio;
}

std::string get_token() {
	char* t = getenv("DPP_UNIT_TEST_TOKEN");
	if (!t) {
		std::cerr << "\u001b[31mDPP_UNIT_TEST_TOKEN not defined -- this is likely a fork.\n\nNot running unit tests.\u001b[0m\n";
		exit(0);
	}
	std::string tok = std::string(t);
	if (tok.empty()) {
		std::cerr << "\u001b[31mDPP_UNIT_TEST_TOKEN empty -- this is likely a PR.\n\nNot running unit tests.\u001b[0m\n";
		exit(0);
	}
	return tok;
}

void wait_for_tests() {
	uint16_t ticks = 0;
	while (ticks < TEST_TIMEOUT) {
		size_t executed = 0;
		for (auto & t : tests) {
			if (t.second.executed == true) {
				executed++;
			}
		}
		if (executed == tests.size()) {
			std::this_thread::sleep_for(std::chrono::seconds(10));
			return;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
		ticks++;
	}
}
