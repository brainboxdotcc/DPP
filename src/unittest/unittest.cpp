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
#include <dpp/dpp.h>
#include <dpp/json.h>

/* Current list of unit tests */
std::map<std::string, test_t> tests = {
	{"CLUSTER", {tt_offline, "Instantiate DPP cluster", false, false}},
	{"BOTSTART", {tt_online, "cluster::start method", false, false}},
	{"CONNECTION", {tt_online, "Connection to client websocket", false, false}},
	{"APPCOMMAND", {tt_online, "Creation of application command", false, false}},
	{"DELCOMMAND", {tt_online, "Deletion of application command", false, false}},
	{"LOGGER", {tt_online, "Log events", false, false}},
	{"MESSAGECREATE", {tt_online, "Creation of a channel message", false, false}},
	{"MESSAGEEDIT", {tt_online, "Editing a channel message", false, false}},
	{"EDITEVENT", {tt_online, "Message edit event", false, false}},
	{"MESSAGEDELETE", {tt_online, "Deletion of a channel message", false, false}},
	{"MESSAGERECEIVE", {tt_online, "Receipt of a created message", false, false}},
	{"MESSAGEFILE", {tt_online, "Message attachment send and check", false, false}},
	{"CACHE", {tt_online, "Test guild cache", false, false}},
	{"USERCACHE", {tt_online, "Test user cache", false, false}},
	{"VOICECONN", {tt_online, "Connect to voice channel", false, false}},
	{"VOICESEND", {tt_online, "Send audio to voice channel", false, false}},
	{"REACT", {tt_online, "React to a message", false, false}},
	{"REACTEVENT", {tt_online, "Reaction event", false, false}},
	{"GUILDCREATE", {tt_online, "Receive guild create event", false, false}},
	{"MESSAGESGET", {tt_online, "Get messages", false, false}},
	{"TIMESTAMP", {tt_online, "crossplatform_strptime()", false, false}},
	{"ICONHASH", {tt_offline, "utility::iconhash", false, false}},
	{"CURRENTUSER", {tt_online, "cluster::current_user_get()", false, false}},
	{"GETGUILD", {tt_online, "cluster::guild_get()", false, false}},
	{"GETCHAN", {tt_online, "cluster::channel_get()", false, false}},
	{"GETCHANS", {tt_online, "cluster::channels_get()", false, false}},
	{"GETROLES", {tt_online, "cluster::roles_get()", false, false}},
	{"GETINVS", {tt_online, "cluster::guild_get_invites()", false, false}},
	{"GETBANS", {tt_online, "cluster::guild_get_bans()", false, false}},
	{"GETPINS", {tt_online, "cluster::channel_pins_get()", false, false}},
	{"GETEVENTS", {tt_online, "cluster::guild_events_get()", false, false}},
	{"GETEVENT", {tt_online, "cluster::guild_event_get()", false, false}},
	{"MSGCREATESEND", {tt_online, "message_create_t::send()", false, false}},
	{"GETEVENTUSERS", {tt_online, "cluster::guild_event_users_get()", false, false}},
	{"TIMERSTART", {tt_online, "start timer", false, false}},
	{"TIMERSTOP", {tt_online, "stop timer", false, false}},
	{"ONESHOT", {tt_online, "one-shot timer", false, false}},
	{"PRESENCE", {tt_online, "Presence intent", false, false}},
	{"CUSTOMCACHE", {tt_offline, "Instantiate a cache", false, false}},
	{"MSGCOLLECT", {tt_online, "message_collector", false, false}},
	{"TS", {tt_online, "managed::get_creation_date()", false, false}},
	{"READFILE", {tt_offline, "utility::read_file()", false, false}},
	{"TIMESTAMPTOSTRING", {tt_offline, "ts_to_string()", false, false}},
	{"TIMESTRINGTOTIMESTAMP", {tt_offline, "ts_not_null()", false, false}},
	{"OPTCHOICE_DOUBLE", {tt_offline, "command_option_choice::fill_from_json: double", false, false}},
	{"OPTCHOICE_INT", {tt_offline, "command_option_choice::fill_from_json: int64_t", false, false}},
	{"OPTCHOICE_BOOL", {tt_offline, "command_option_choice::fill_from_json: bool", false, false}},
	{"OPTCHOICE_SNOWFLAKE", {tt_offline, "command_option_choice::fill_from_json: snowflake", false, false}},
	{"OPTCHOICE_STRING", {tt_offline, "command_option_choice::fill_from_json: string", false, false}},
	{"HOSTINFO", {tt_offline, "https_client::get_host_info()", false, false}},
	{"HTTPS", {tt_online, "https_client HTTPS request", false, false}},
	{"HTTP", {tt_offline, "https_client HTTP request", false, false}},
	{"MULTIHEADER", {tt_offline, "multiheader cookie test", false, false}},
	{"RUNONCE", {tt_offline, "run_once<T>", false, false}},
	{"WEBHOOK", {tt_offline, "webhook construct from URL", false, false}},
	{"MD_ESC_1", {tt_offline, "Markdown escaping (ignore code block contents)", false, false}},
	{"MD_ESC_2", {tt_offline, "Markdown escaping (escape code block contents)", false, false}},
	{"URLENC", {tt_offline, "URL encoding", false, false}},
	{"SYNC", {tt_online, "sync<T>()", false, false}},
	{"COMPARISON", {tt_offline, "manged object comparison", false, false}},
	{"CHANNELCACHE", {tt_online, "find_channel()", false, false}},
	{"CHANNELTYPES", {tt_online, "channel type flags", false, false}},
	{"FORUM_CREATION", {tt_online, "create a forum channel", false, false}},
	{"FORUM_CHANNEL_GET", {tt_online, "retrieve the created forum channel", false, false}},
	{"FORUM_CHANNEL_DELETE", {tt_online, "delete the created forum channel", false, false}},

	{"GUILD_BAN_CREATE", {tt_online, "cluster::guild_ban_add ban three deleted discord accounts", false, false}},
	{"GUILD_BAN_GET", {tt_online, "cluster::guild_get_ban getting one of the banned accounts", false, false}},
	{"GUILD_BANS_GET", {tt_online, "cluster::guild_get_bans get bans using the after-parameter", false, false}},
	{"GUILD_BAN_DELETE", {tt_online, "cluster::guild_ban_delete unban the banned discord accounts", false, false}},

	{"THREAD_CREATE", {tt_online, "cluster::thread_create", false, false}},
	{"THREAD_CREATE_EVENT", {tt_online, "cluster::on_thread_create event", false, false}},
	{"THREAD_DELETE", {tt_online, "cluster::channel_delete with thread", false, false}},
	{"THREAD_DELETE_EVENT", {tt_online, "cluster::on_thread_delete event", false, false}},
	{"THREAD_EDIT", {tt_online, "cluster::thread_edit", false, false}},
	{"THREAD_UPDATE_EVENT", {tt_online, "cluster::on_thread_update event", false, false}},
	{"THREAD_GET_ACTIVE", {tt_online, "cluster::threads_get_active", false, false}},

	{"VOICE_CHANNEL_CREATE", {tt_online, "creating a voice channel", false, false}},
	{"VOICE_CHANNEL_EDIT", {tt_online, "editing the created voice channel", false, false}},
	{"VOICE_CHANNEL_DELETE", {tt_online, "deleting the created voice channel", false, false}},

	{"PERMISSION_CLASS", {tt_offline, "permission", false, false}},
	{"USER_GET", {tt_online, "cluster::user_get", false, false}},
	{"USER_GET_FLAGS", {tt_online, "cluster::user_get flag parsing", false, false}},
	{"MEMBER_GET", {tt_online, "cluster::guild_get_member", false, false}},
	{"USER.GET_MENTION", {tt_offline, "user::get_mention", false, false}},
	{"USER.FORMAT_USERNAME", {tt_offline, "user::format_username", false, false}},
	{"USER.GET_CREATION_TIME", {tt_offline, "user::get_creation_time", false, false}},
	{"USER.GET_AVATAR_URL", {tt_offline, "user::get_avatar_url", false, false}},
	{"CHANNEL.SET_TYPE", {tt_offline, "channel::set_type", false, false}},
	{"CHANNEL.GET_MENTION", {tt_offline, "channel::get_mention", false, false}},
	{"UTILITY.ICONHASH", {tt_offline, "utility::iconhash", false, false}},
	{"UTILITY.MAKE_URL_PARAMETERS", {tt_offline, "utility::make_url_parameters", false, false}},
	{"UTILITY.MARKDOWN_ESCAPE", {tt_offline, "utility::markdown_escape", false, false}},
	{"UTILITY.TOKENIZE", {tt_offline, "utility::tokenize", false, false}},
	{"UTILITY.URL_ENCODE", {tt_offline, "utility::url_encode", false, false}},
	{"UTILITY.SLASHCOMMAND_MENTION", {tt_offline, "utility::slashcommand_mention", false, false}},
	{"UTILITY.CHANNEL_MENTION", {tt_offline, "utility::channel_mention", false, false}},
	{"UTILITY.USER_MENTION", {tt_offline, "utility::user_mention", false, false}},
	{"UTILITY.ROLE_MENTION", {tt_offline, "utility::role_mention", false, false}},
	{"UTILITY.EMOJI_MENTION", {tt_offline, "utility::emoji_mention", false, false}},
	{"UTILITY.AVATAR_SIZE", {tt_offline, "utility::avatar_size", false, false}},
	{"UTILITY.CDN_ENDPOINT_URL_HASH", {tt_offline, "utility::cdn_endpoint_url_hash", false, false}},
	{"STICKER.GET_URL", {tt_offline, "sticker::get_url aka utility::cdn_endpoint_url_sticker", false, false}},
	{"EMOJI.GET_URL", {tt_offline, "emoji::get_url", false, false}},
	{"ROLE.COMPARE", {tt_offline, "role::operator<", false, false}},
	{"ROLE_CREATE", {tt_online, "cluster::role_create", false, false}},
	{"ROLE_EDIT", {tt_online, "cluster::role_edit", false, false}},
	{"ROLE_DELETE", {tt_online, "cluster::role_delete", false, false}},
	{"JSON_PARSE_ERROR", {tt_online, "JSON parse error for post_rest", false, false}},
	{"USER_GET_CACHED_PRESENT", {tt_online, "cluster::user_get_cached_sync() with present member", false, false}},
	{"USER_GET_CACHED_ABSENT", {tt_online, "cluster::user_get_cached_sync() with not present member", false, false}},
	{"GET_PARAMETER_WITH_SUBCOMMANDS", {tt_offline, "interaction_create_t::get_parameter() with subcommands", false, false}},
	{"GET_PARAMETER_WITHOUT_SUBCOMMANDS", {tt_offline, "interaction_create_t::get_parameter() without subcommands", false, false}},
	{"AUTOMOD_RULE_CREATE", {tt_online, "cluster::automod_rule_create", false, false}},
	{"AUTOMOD_RULE_GET", {tt_online, "cluster::automod_rule_get", false, false}},
	{"AUTOMOD_RULE_GET_ALL", {tt_online, "cluster::automod_rules_get", false, false}},
	{"AUTOMOD_RULE_DELETE", {tt_online, "cluster::automod_rule_delete", false, false}},
	{"REQUEST_GET_IMAGE", {tt_online, "using the cluster::request method to fetch an image", false, false}},
	{"EMOJI_CREATE", {tt_online, "cluster::guild_emoji_create", false, false}},
	{"EMOJI_GET", {tt_online, "cluster::guild_emoji_get", false, false}},
	{"EMOJI_DELETE", {tt_online, "cluster::guild_emoji_delete", false, false}},
	{"INVITE_CREATE_EVENT", {tt_online, "cluster::on_invite_create", false, false}},
	{"INVITE_DELETE_EVENT", {tt_online, "cluster::on_invite_delete", false, false}},
	{"INVITE_CREATE", {tt_online, "cluster::channel_invite_create", false, false}},
	{"INVITE_GET", {tt_online, "cluster::invite_get", false, false}},
	{"INVITE_DELETE", {tt_online, "cluster::invite_delete", false, false}},

	/* Extended set -- Less important, skipped on the master branch due to rate limits and GitHub actions limitations*/
	/* To execute, run unittests with "full" command line argument */
	{"MESSAGEPIN", {tt_extended, "Pinning a channel message", false, false}},
	{"MESSAGEUNPIN", {tt_extended, "Unpinning a channel message", false, false}},

	{"THREAD_MEMBER_ADD", {tt_extended, "cluster::thread_member_add", false, false}},
	{"THREAD_MEMBER_GET", {tt_extended, "cluster::thread_member_get", false, false}},
	{"THREAD_MEMBERS_GET", {tt_extended, "cluster::thread_members_get", false, false}},
	{"THREAD_MEMBER_REMOVE", {tt_extended, "cluster::thread_member_remove", false, false}},
	{"THREAD_MEMBERS_ADD_EVENT", {tt_extended, "cluster::on_thread_members_update event with member addition", false, false}},
	{"THREAD_MEMBERS_REMOVE_EVENT", {tt_extended, "cluster::on_thread_members_update event with member removal", false, false}},
	{"THREAD_CREATE_MESSAGE", {tt_extended, "cluster::thread_create_with_message", false, false}},

	{"THREAD_MESSAGE", {tt_extended, "message manipulation in thread", false, false}},
	{"THREAD_MESSAGE_CREATE_EVENT", {tt_extended, "cluster::on_message_create in thread", false, false}},
	{"THREAD_MESSAGE_EDIT_EVENT", {tt_extended, "cluster::on_message_edit in thread", false, false}},
	{"THREAD_MESSAGE_DELETE_EVENT", {tt_extended, "cluster::on_message_delete in thread", false, false}},
	{"THREAD_MESSAGE_REACT_ADD_EVENT", {tt_extended, "cluster::on_reaction_add in thread", false, false}},
	{"THREAD_MESSAGE_REACT_REMOVE_EVENT", {tt_extended, "cluster::on_reaction_remove in thread", false, false}},
};

double start = dpp::utility::time_f();
bool offline = false;
bool extended = false;

dpp::snowflake TEST_GUILD_ID = std::stoull(SAFE_GETENV("TEST_GUILD_ID"));
dpp::snowflake TEST_TEXT_CHANNEL_ID = std::stoull(SAFE_GETENV("TEST_TEXT_CHANNEL_ID"));
dpp::snowflake TEST_VC_ID = std::stoull(SAFE_GETENV("TEST_VC_ID"));
dpp::snowflake TEST_USER_ID = std::stoull(SAFE_GETENV("TEST_USER_ID"));
dpp::snowflake TEST_EVENT_ID = std::stoull(SAFE_GETENV("TEST_EVENT_ID"));

void set_test(const std::string &testname, bool success) {
	auto i = tests.find(testname);
	if (i != tests.end()) {
		if (offline && i->second.type == tt_online) {
			i->second.success = true;
			i->second.executed = true;
			std::cout << "[" << std::fixed << std::setprecision(3)  << get_time() << "]: " << "[\u001b[33mSKIPPED\u001b[0m] " << i->second.description << "\n";
		} else {
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
}

double get_start_time() {
	return start;
}

double get_time() {
	return dpp::utility::time_f() - get_start_time();
}

int test_summary() {
	/* Report on all test cases */
	int failed = 0, passed = 0, skipped = 0;
	std::cout << "\u001b[37;1m\n\nUNIT TEST SUMMARY\n==================\n\u001b[0m";
	for (auto & t : tests) {
		bool test_skipped = false;
		if ((t.second.type == tt_online && offline) || (t.second.type == tt_extended && !extended)) {
			skipped++;
			test_skipped = true;
		} else {
			if (t.second.success == false || t.second.executed == false) {
				failed++;
			} else {
				passed++;
			}
		}
		std::cout << std::left << std::setw(60) << t.second.description << " " << std::fixed << std::setw(6) << (test_skipped ? "\u001b[33mSKIPPED" : (t.second.executed && t.second.success ? "\u001b[32mPASS" : (!t.second.executed ? "\u001b[31mNOT EXECUTED" : "\u001b[31mFAIL"))) << std::setw(0) << "\u001b[0m\n";
	}
	std::cout << "\u001b[37;1m\nExecution finished in " << std::fixed << std::setprecision(3) <<  get_time() << std::setprecision(0) << " seconds.\nFailed: " << failed << " Passed: " << passed << (skipped ? " Skipped: " : "") << (skipped ? std::to_string(skipped) : "") << " Percentage: " << std::setprecision(2) << ((float)(passed) / (float)(passed + failed) * 100.0f) << "%\u001b[0m\n";
	return failed;
}

namespace {
	std::string get_testdata_dir() {
		char *env_var = getenv("TEST_DATA_DIR");

		return (env_var ? env_var : "../../testdata/");
	}
} // namespace

std::vector<uint8_t> load_test_audio() {
	std::vector<uint8_t> testaudio;
	std::string dir = get_testdata_dir();
	std::ifstream input (dir + "Robot.pcm", std::ios::in|std::ios::binary|std::ios::ate);
	if (input.is_open()) {
		size_t testaudio_size = input.tellg();
		testaudio.resize(testaudio_size);
		input.seekg(0, std::ios::beg);
		input.read((char*)testaudio.data(), testaudio_size);
		input.close();
	}
	else {
		std::cout << "ERROR: Can't load " + dir + "Robot.pcm\n";
		exit(1);
	}
	return testaudio;
}

std::vector<uint8_t> load_test_image() {
	std::vector<uint8_t> testimage;
	std::string dir = get_testdata_dir();
	std::ifstream input (dir + "DPP-Logo.png", std::ios::in|std::ios::binary|std::ios::ate);
	if (input.is_open()) {
		size_t testimage_size = input.tellg();
		testimage.resize(testimage_size);
		input.seekg(0, std::ios::beg);
		input.read((char*)testimage.data(), testimage_size);
		input.close();
	}
	else {
		std::cout << "ERROR: Can't load " + dir + "DPP-Logo.png\n";
		exit(1);
	}
	return testimage;
}

std::string get_token() {
	char* t = getenv("DPP_UNIT_TEST_TOKEN");
	std::string tok;
	if (!t) {
		offline = true;
	}  else {
		tok = std::string(t);
		if (tok.empty()) {
			offline = true;
		}
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
			} else if (!t.second.executed && ((offline && t.second.type == tt_online) || (!extended && t.second.type == tt_extended))) {
				executed++;
				t.second.executed = true;
				std::cout << "[" << std::fixed << std::setprecision(3)  << get_time() << "]: " << "[\u001b[33mSKIPPED\u001b[0m] " << t.second.description << "\n";
			}
		}
		if (executed == tests.size()) {
			std::this_thread::sleep_for(std::chrono::seconds(10));
			return;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
		ticks++;
	}
	for (auto &t : tests) {
		if (!t.second.executed)
			std::cout << "[" << std::fixed << std::setprecision(3)  << get_time() << "]: " << "[\u001b[31mTIMEOUT\u001b[0m] " << t.second.description << "\n";
	}
}
