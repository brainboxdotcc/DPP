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
#undef DPP_BUILD
#ifdef _WIN32
_Pragma("warning( disable : 4251 )"); // 4251 warns when we export classes or structures with stl member variables
#endif
#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt/format.h>
 
using json = nlohmann::json;

/* Represents a test case */
struct test_t {
	/* Description of test */
	std::string description;
	/* Has been executed */
	bool executed = false;
	/* Was successfully tested */
	bool success = false;
};

/* How long the unit tests can run for */
const int64_t TEST_TIMEOUT = 60;

/* IDs of various channels and guilds used to test */
const dpp::snowflake TEST_GUILD_ID = 825407338755653642;
const dpp::snowflake TEST_TEXT_CHANNEL_ID = 828681546533437471;
const dpp::snowflake TEST_VC_ID = 825411635631095858;
const dpp::snowflake TEST_USER_ID = 826535422381391913;

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
};

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
void set_test(const std::string testname, bool success = false) {
	auto i = tests.find(testname);
	if (i != tests.end()) {
		if (!i->second.executed) {
			std::cout << "[\u001b[33mTESTING\u001b[0m] " << i->second.description << "\n";
		} else if (!success) {
			std::cout << "[\u001b[31mFAILED\u001b[0m] " << i->second.description << "\n";
		}
		i->second.executed = true;
		if (success) {
			i->second.success = true;
			std::cout << "[\u001b[32mSUCCESS\u001b[0m] " << i->second.description << "\n";
		}
	}
}

/* Unit tests go here */
int main()
{
	char* t = getenv("DPP_UNIT_TEST_TOKEN");
	if (!t) {
		std::cerr << "\u001b[31mDPP_UNIT_TEST_TOKEN not defined -- this is likely a fork.\n\nNot running unit tests.\u001b[0m\n";
		return 0;
	}
	std::string token(t);

	uint8_t* testaudio = nullptr;
	size_t testaudio_size = 0;
	std::ifstream input ("../testdata/Robot.pcm", std::ios::in|std::ios::binary|std::ios::ate);
	if (input.is_open()) {
		testaudio_size = input.tellg();
		testaudio = new uint8_t[testaudio_size];
		input.seekg (0, std::ios::beg);
		input.read ((char*)testaudio, testaudio_size);
		input.close();
	}

	set_test("CLUSTER", false);
	try {
		set_test("CLUSTER", true);
		dpp::cluster bot(token);
		set_test("CONNECTION", false);
		set_test("GUILDCREATE", false);

		/* This ensures we test both protocols, as voice is json and shard is etf */
		bot.set_websocket_protocol(dpp::ws_etf);

		bot.on_ready([&bot](const dpp::ready_t & event) {

			set_test("CONNECTION", true);
			set_test("APPCOMMAND", false);
			set_test("LOGGER", false);

			bot.log(dpp::ll_info, "Test log message");

			bot.guild_command_create(dpp::slashcommand().set_name("testcommand")
				.set_description("Test command for DPP unit test")
				.set_application_id(bot.me.id),
				TEST_GUILD_ID, [&bot](const dpp::confirmation_callback_t &callback) {
					if (!callback.is_error()) {
						set_test("APPCOMMAND", true);
						set_test("DELCOMMAND", false);
						dpp::slashcommand s = std::get<dpp::slashcommand>(callback.value);
						bot.guild_command_delete(s.id, TEST_GUILD_ID, [&bot](const dpp::confirmation_callback_t &callback) {
							if (!callback.is_error()) {
								set_test("DELCOMMAND", true);
								set_test("MESSAGECREATE", false);
								set_test("MESSAGERECEIVE", false);
								bot.message_create(dpp::message(TEST_TEXT_CHANNEL_ID, "test message"), [&bot](const dpp::confirmation_callback_t &callback) {
									if (!callback.is_error()) {
										set_test("MESSAGECREATE", true);
										set_test("REACT", false);
										set_test("MESSAGEDELETE", false);
										dpp::message m = std::get<dpp::message>(callback.value);
										set_test("REACTEVENT", false);
										bot.message_add_reaction(m.id, TEST_TEXT_CHANNEL_ID, "😄", [&bot](const dpp::confirmation_callback_t &callback) {
											if (!callback.is_error()) {
												set_test("REACT", true);
											} else {
												set_test("REACT", false);
											}
										});
										bot.message_delete(m.id, TEST_TEXT_CHANNEL_ID, [&bot](const dpp::confirmation_callback_t &callback) {

											if (!callback.is_error()) {
												set_test("MESSAGEDELETE", true);
												set_test("CACHE", false);

												dpp::guild* g = dpp::find_guild(TEST_GUILD_ID);

												if (g) {
													set_test("CACHE", true);
													set_test("VOICECONN", false);
													dpp::discord_client* s = bot.get_shard(0);
													s->connect_voice(g->id, TEST_VC_ID, false, false);
												} else {
													set_test("CACHE", false);
												}
											} else {
												set_test("MESSAGEDELETE", false);
											}
										});
									} else {
										set_test("MESSAGECREATE", false);
									}
								});
							}
						});

					}
				});
		});

		bot.on_log([&](const dpp::log_t & event) {
			std::cout << dpp::utility::loglevel(event.severity) << ": " << event.message << "\n";
			if (event.message == "Test log message") {
				set_test("LOGGER", true);
			}
		});

		bot.on_message_reaction_add([&](const dpp::message_reaction_add_t & event) {
			if (event.reacting_user.id == bot.me.id && event.reacting_emoji.name == "😄") {
				set_test("REACTEVENT", true);
			}
		});

		bot.on_voice_ready([&](const dpp::voice_ready_t & event) {
			set_test("VOICECONN", true);
			dpp::discord_voice_client* v = event.voice_client;
			set_test("VOICESEND", false);
			if (v && v->is_ready()) {
				v->send_audio_raw((uint16_t*)testaudio, testaudio_size);
			} else {
				set_test("VOICESEND", false);
			}
		});

		bot.on_voice_buffer_send([&](const dpp::voice_buffer_send_t & event) {
			if (event.buffer_size == 0) {
				set_test("VOICESEND", true);
			}
		});

		bot.on_guild_create([&](const dpp::guild_create_t & event) {
			if (event.created->id == TEST_GUILD_ID) {
				set_test("GUILDCREATE", true);
			}
		});

		bot.on_message_create([&](const dpp::message_create_t & event) {
			if (event.msg->author->id == bot.me.id) {
				set_test("MESSAGERECEIVE", true);
				set_test("MESSAGESGET", false);
				bot.messages_get(event.msg->channel_id, 0, event.msg->id, 0, 5, [](const dpp::confirmation_callback_t &cc){
					if (!cc.is_error()) {
						set_test("MESSAGESGET", true);
					}  else {
						set_test("MESSAGESGET", false);
					}
				});
			}
		});

		set_test("BOTSTART", false);
		try {
			bot.start(true);
			set_test("BOTSTART", true);
		}
		catch (const std::exception & e) {
			set_test("BOTSTART", false);
		}

		std::this_thread::sleep_for(std::chrono::seconds(TEST_TIMEOUT / 2));

		set_test("USERCACHE", false);
		dpp::user* u = dpp::find_user(TEST_USER_ID);
		set_test("USERCACHE", u);

		std::this_thread::sleep_for(std::chrono::seconds(TEST_TIMEOUT / 2));

	}
	catch (const std::exception & e) {
		set_test("CLUSTER", false);
	}

	/* Report on all test cases */
	uint16_t failed = 0, passed = 0;
	fmt::print("\u001b[37;1m\n\nUNIT TEST SUMMARY\n==================\n\u001b[0m");
	for (auto & t : tests) {
		if (t.second.success == false || t.second.executed == false) {
			failed++;
		} else {
			passed++;
		}
		fmt::print("{:50s} {:6s}\u001b[0m\n", t.second.description, t.second.executed && t.second.success ? "\u001b[32mPASS" : "\u001b[31mFAIL");
	}
	fmt::print("\u001b[37;1m\nFailed: {} Passed: {} Percentage: {:.02f}%\u001b[0m\n", failed, passed, (float)(passed) / (float)(tests.size()) * 100.0f);

	/* Return value = number of failed tests, exit code 0 = success */
	return failed;
}
