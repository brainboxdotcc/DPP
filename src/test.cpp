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
 
/* Unit tests go here */
int main()
{
	std::string token(get_token());
	std::vector<uint8_t> testaudio = load_test_audio();

	set_test("CLUSTER", false);
	try {
		dpp::cluster bot(token);
		set_test("CLUSTER", true);
		set_test("CONNECTION", false);
		set_test("GUILDCREATE", false);
		set_test("ICONHASH", false);

		dpp::utility::iconhash i;
		std::string dummyval("fcffffffffffff55acaaaaaaaaaaaa66");
		i = dummyval;
		set_test("ICONHASH", (i.to_string() == dummyval));

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
										bot.message_add_reaction(m.id, TEST_TEXT_CHANNEL_ID, "😄", [](const dpp::confirmation_callback_t &callback) {
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

		std::mutex loglock;
		bot.on_log([&](const dpp::log_t & event) {
			std::lock_guard<std::mutex> locker(loglock);
			if (event.severity > dpp::ll_trace) {
				std::cout << "[" << fmt::format("{:3.03f}", dpp::utility::time_f() - get_start_time()) << "]: " << dpp::utility::loglevel(event.severity) << ": " << event.message << "\n";
			}
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
				v->send_audio_raw((uint16_t*)testaudio.data(), testaudio.size());
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

		bool message_tested = false;
		bot.on_message_create([&](const dpp::message_create_t & event) {
			if (event.msg->author->id == bot.me.id && !message_tested) {
				message_tested = true;
				set_test("MESSAGERECEIVE", true);
				set_test("MESSAGESGET", false);
				bot.messages_get(event.msg->channel_id, 0, event.msg->id, 0, 5, [](const dpp::confirmation_callback_t &cc){
					if (!cc.is_error()) {
						dpp::message_map mm = std::get<dpp::message_map>(cc.value);
						if (mm.size()) {
							set_test("MESSAGESGET", true);
							set_test("TIMESTAMP", false);
							dpp::message m = mm.begin()->second;
							if (m.sent > 0) {
								set_test("TIMESTAMP", true);
							} else {
								set_test("TIMESTAMP", false);
							}
						} else {
							set_test("MESSAGESGET", false);	
						}
					}  else {
						set_test("MESSAGESGET", false);
					}
				});
				set_test("MSGCREATESEND", false);
				event.send("MSGCREATESEND", [&bot, ch_id = event.msg->channel_id] (const auto& cc) {
					if (!cc.is_error()) {
						dpp::message m = std::get<dpp::message>(cc.value);
						if (m.channel_id == ch_id) {
							set_test("MSGCREATESEND", true);
						} else {
							set_test("MSGCREATESEND", false);
						}
						bot.message_delete(m.id, m.channel_id);
					} else { 
						set_test("MSGCREATESEND", false);
					}
				});
				set_test("MSGCREATEREPLY", false);
				set_test("MSGMENTIONUSER", false);
				event.reply("MSGCREATEREPLY", true, [&bot, ref_id = event.msg->id, author_id = event.msg->author->id] (const auto& cc) {
					if (!cc.is_error()) {
						dpp::message m = std::get<dpp::message>(cc.value);
						if (m.message_reference.message_id == ref_id) {
							bool f = false;
							for (auto&[usr, mem] : m.mentions) {
								if (usr.id == author_id) {
									set_test("MSGMENTIONUSER", true);
									f = true;
									break;
								}
							}
							if (!f) {
								set_test("MSGMENTIONUSER", false);
							}
							set_test("MSGCREATEREPLY", true);
						} else {
							set_test("MSGCREATEREPLY", false);
							std::cout << " *** " << ref_id << " -> " << m.message_reference.message_id << "\n";
						}
						bot.message_delete(m.id, m.channel_id);
					} else { 
						set_test("MSGCREATEREPLY", false);
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

		noparam_api_test(current_user_get, dpp::user_identified, "CURRENTUSER");
		singleparam_api_test(channel_get, TEST_TEXT_CHANNEL_ID, dpp::channel, "GETCHAN");
		singleparam_api_test(guild_get, TEST_GUILD_ID, dpp::guild, "GETGUILD");
		singleparam_api_test_list(roles_get, TEST_GUILD_ID, dpp::role_map, "GETROLES");
		singleparam_api_test_list(channels_get, TEST_GUILD_ID, dpp::channel_map, "GETCHANS");
		singleparam_api_test_list(guild_get_invites, TEST_GUILD_ID, dpp::invite_map, "GETINVS");
		singleparam_api_test_list(guild_get_bans, TEST_GUILD_ID, dpp::ban_map, "GETBANS");
		singleparam_api_test_list(channel_pins_get, TEST_TEXT_CHANNEL_ID, dpp::message_map, "GETPINS");
		singleparam_api_test_list(guild_events_get, TEST_GUILD_ID, dpp::scheduled_event_map, "GETEVENTS");
		twoparam_api_test(guild_event_get, TEST_GUILD_ID, TEST_EVENT_ID, dpp::scheduled_event, "GETEVENT");
		twoparam_api_test_list(guild_event_users_get, TEST_GUILD_ID, TEST_EVENT_ID, dpp::user_map, "GETEVENTUSERS");

		std::this_thread::sleep_for(std::chrono::seconds(5));

		set_test("USERCACHE", false);
		dpp::user* u = dpp::find_user(TEST_USER_ID);
		set_test("USERCACHE", u);

		wait_for_tests();

	}
	catch (const std::exception & e) {
		set_test("CLUSTER", false);
	}

	/* Return value = number of failed tests, exit code 0 = success */
	return test_summary();
}
