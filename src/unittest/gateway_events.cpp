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
#include <dpp/unicode_emoji.h>

/* Unit tests for Gateway events */
void gateway_events_tests(const std::string& token, dpp::cluster& bot) {
	std::vector<uint8_t> test_image = load_test_image();
	std::vector<uint8_t> testaudio = load_test_audio();

	set_test(PRESENCE, false);
	set_test(CLUSTER, false);
	try {
		bot.set_websocket_protocol(dpp::ws_etf);
		set_test(CLUSTER, true);
		set_test(CONNECTION, false);
		set_test(GUILDCREATE, false);
		set_test(ICONHASH, false);

		set_test(MSGCOLLECT, false);
		if (!offline) {
			/* Intentional leak: freed on unit test end */
			[[maybe_unused]]
			message_collector* collect_messages = new message_collector(&bot, 25);
		}

		set_test(JSON_PARSE_ERROR, false);
		dpp::rest_request<dpp::confirmation>(&bot, "/nonexistent", "address", "", dpp::m_get, "", [](const dpp::confirmation_callback_t& e) {
			if (e.is_error() && e.get_error().code == 404) {
				set_test(JSON_PARSE_ERROR, true);
			} else {
				set_test(JSON_PARSE_ERROR, false);
			}
		});

		dpp::utility::iconhash i;
		std::string dummyval("fcffffffffffff55acaaaaaaaaaaaa66");
		i = dummyval;
		set_test(ICONHASH, (i.to_string() == dummyval));

		/* This ensures we test both protocols, as voice is json and shard is etf */
		bot.set_websocket_protocol(dpp::ws_etf);

		bot.on_form_submit([&](const dpp::form_submit_t & event) {
		});

		/* This is near impossible to test without a 'clean room' voice channel.
		 * We attach this event just so that the decoder events are fired while we
		 * are sending audio later, this way if the audio receive code is plain unstable
		 * the test suite will crash and fail.
		 */
		bot.on_voice_receive_combined([&](const auto& event) {
		});

		std::promise<void> ready_promise;
		std::future ready_future = ready_promise.get_future();
		bot.on_ready([&](const dpp::ready_t & event) {
			set_test(CONNECTION, true);
			ready_promise.set_value();

			set_test(APPCOMMAND, false);
			set_test(LOGGER, false);
			bot.log(dpp::ll_info, "Test log message");

			bot.guild_command_create(dpp::slashcommand().set_name("testcommand")
				.set_description("Test command for DPP unit test")
				.add_option(dpp::command_option(dpp::co_attachment, "file", "a file"))
				.set_application_id(bot.me.id)
				.add_localization("fr", "zut", "Ou est la salor dans Discord?"),
				TEST_GUILD_ID, [&](const dpp::confirmation_callback_t &callback) {
					if (!callback.is_error()) {
						set_test(APPCOMMAND, true);
						set_test(DELCOMMAND, false);
						dpp::slashcommand s = std::get<dpp::slashcommand>(callback.value);
						bot.guild_command_delete(s.id, TEST_GUILD_ID, [&](const dpp::confirmation_callback_t &callback) {
							if (!callback.is_error()) {
								dpp::message test_message(TEST_TEXT_CHANNEL_ID, "test message");

								set_test(DELCOMMAND, true);
								set_test(MESSAGECREATE, false);
								set_test(MESSAGEEDIT, false);
								set_test(MESSAGERECEIVE, false);
								test_message.add_file("no-mime", "test");
								test_message.add_file("test.txt", "test", "text/plain");
								test_message.add_file("test.png", std::string{test_image.begin(), test_image.end()}, "image/png");
								bot.message_create(test_message, [&bot](const dpp::confirmation_callback_t &callback) {
									if (!callback.is_error()) {
										set_test(MESSAGECREATE, true);
										set_test(REACT, false);
										dpp::message m = std::get<dpp::message>(callback.value);
										set_test(REACTEVENT, false);
										bot.message_add_reaction(m.id, TEST_TEXT_CHANNEL_ID, "😄", [](const dpp::confirmation_callback_t &callback) {
											if (!callback.is_error()) {
												set_test(REACT, true);
											} else {
												set_test(REACT, false);
											}
										});
										set_test(EDITEVENT, false);
										bot.message_edit(dpp::message(m).set_content("test edit"), [](const dpp::confirmation_callback_t &callback) {
											if (!callback.is_error()) {
												set_test(MESSAGEEDIT, true);
											}
										});
									}
								});
							} else {
								set_test(DELCOMMAND, false);
							}
						});
					}
			});
		});

		std::mutex loglock;
		bot.on_log([&](const dpp::log_t & event) {
			std::lock_guard<std::mutex> locker(loglock);
			if (event.severity > dpp::ll_trace) {
				std::cout << "[" << std::fixed << std::setprecision(3) << (dpp::utility::time_f() - get_start_time()) << "]: [\u001b[36m" << dpp::utility::loglevel(event.severity) << "\u001b[0m] " << event.message << "\n";
			}
			if (event.message == "Test log message") {
				set_test(LOGGER, true);
			}
		});

		set_test(RUNONCE, false);
		uint8_t runs = 0;
		for (int x = 0; x < 10; ++x) {
			if (dpp::run_once<struct test_run>()) {
				runs++;
			}
		}
		set_test(RUNONCE, (runs == 1));

		bot.on_voice_ready([&](const dpp::voice_ready_t & event) {
			set_test(VOICECONN, true);
			dpp::discord_voice_client* v = event.voice_client;
			set_test(VOICESEND, false);
			if (v && v->is_ready()) {
				v->send_audio_raw(reinterpret_cast<uint16_t*>(testaudio.data()), testaudio.size()); // TODO: fix type punning
			} else {
				set_test(VOICESEND, false);
			}
		});

		bot.on_invite_create([](const dpp::invite_create_t &event) {
			auto &inv = event.created_invite;
			if (!inv.code.empty() && inv.channel_id == TEST_TEXT_CHANNEL_ID && inv.guild_id == TEST_GUILD_ID && inv.created_at != 0 && inv.max_uses == 100) {
				set_test(INVITE_CREATE_EVENT, true);
			}
		});

		bot.on_invite_delete([](const dpp::invite_delete_t &event) {
			auto &inv = event.deleted_invite;
			if (!inv.code.empty() && inv.channel_id == TEST_TEXT_CHANNEL_ID && inv.guild_id == TEST_GUILD_ID) {
				set_test(INVITE_DELETE_EVENT, true);
			}
		});

		bot.on_voice_buffer_send([&](const dpp::voice_buffer_send_t & event) {
			if (event.buffer_size == 0) {
				set_test(VOICESEND, true);
			}
		});

		set_test(SYNC, false);
		if (!offline) {
			dpp::message m = dpp::sync<dpp::message>(&bot, &dpp::cluster::message_create, dpp::message(TEST_TEXT_CHANNEL_ID, "TEST"));
			set_test(SYNC, m.content == "TEST");
		}

		bot.on_guild_create([&](const dpp::guild_create_t & event) {
			if (event.created->id == TEST_GUILD_ID) {
				set_test(GUILDCREATE, true);
				if (event.presences.size() && event.presences.begin()->second.user_id > 0) {
					set_test(PRESENCE, true);
				}
				dpp::guild* g = dpp::find_guild(TEST_GUILD_ID);
				set_test(CACHE, false);
				if (g) {
					set_test(CACHE, true);
					set_test(VOICECONN, false);
					dpp::discord_client* s = bot.get_shard(0);
					s->connect_voice(g->id, TEST_VC_ID, false, false);
				}
				else {
					set_test(CACHE, false);
				}
			}
		});

		// this helper class contains logic for the message tests, deletes the message when all tests are done
		class message_test_helper {
		private:
			std::mutex mutex;
			bool pin_tested = false;
			bool thread_tested = false;
			std::array<bool, 3> files_tested{};
			std::array<bool, 3> files_success{};
			dpp::snowflake channel_id;
			dpp::snowflake message_id;
			dpp::cluster &bot;

			void delete_message_if_done() {
				if (files_tested == std::array{true, true, true} && pin_tested && thread_tested) {
					set_test(MESSAGEDELETE, false);
					bot.message_delete(message_id, channel_id, [](const dpp::confirmation_callback_t &callback) {
					if (!callback.is_error()) {
						set_test(MESSAGEDELETE, true);
					}
					});
				}
			}

			void set_pin_tested() {
				assert(!pin_tested);
				pin_tested = true;
				delete_message_if_done();
			}

			void set_thread_tested() {
				assert(!thread_tested);
				thread_tested = true;
				delete_message_if_done();
			}

			void set_file_tested(size_t index) {
				assert(!files_tested[index]);
				files_tested[index] = true;
				if (files_tested == std::array{true, true, true}) {
					set_test(MESSAGEFILE, files_success == std::array{true, true, true});
				}
				delete_message_if_done();
			}

			void test_threads(const dpp::message &message) {
				set_test(THREAD_CREATE_MESSAGE, false);
				set_test(THREAD_DELETE, false);
				set_test(THREAD_DELETE_EVENT, false);
				bot.thread_create_with_message("test", message.channel_id, message.id, 60, 60, [this](const dpp::confirmation_callback_t &callback) {
					std::lock_guard lock(mutex);
					if (callback.is_error()) {
						set_thread_tested();
					}
					else {
						auto thread = callback.get<dpp::thread>();
						thread_id = thread.id;
						set_test(THREAD_CREATE_MESSAGE, true);
						bot.channel_delete(thread.id, [this](const dpp::confirmation_callback_t &callback) {
							set_test(THREAD_DELETE, !callback.is_error());
							set_thread_tested();
						});
					}
				});
			}

			void test_files(const dpp::message &message) {
				set_test(MESSAGEFILE, false);
				if (message.attachments.size() == 3) {
					static constexpr auto check_mimetype = [](const auto &headers, std::string mimetype) {
						if (auto it = headers.find("content-type"); it != headers.end()) {
							// check that the mime type starts with what we gave : for example discord will change "text/plain" to "text/plain; charset=UTF-8"
							return it->second.size() >= mimetype.size() && std::equal(it->second.begin(), it->second.begin() + mimetype.size(), mimetype.begin());
						}
						else {
							return false;
						}
					};
					message.attachments[0].download([&](const dpp::http_request_completion_t &callback) {
						std::lock_guard lock(mutex);
						if (callback.status == 200 && callback.body == "test") {
							files_success[0] = true;
						}
						set_file_tested(0);
					});
					message.attachments[1].download([&](const dpp::http_request_completion_t &callback) {
						std::lock_guard lock(mutex);
						if (callback.status == 200 && check_mimetype(callback.headers, "text/plain") && callback.body == "test") {
							files_success[1] = true;
						}
						set_file_tested(1);
					});
					message.attachments[2].download([&](const dpp::http_request_completion_t &callback) {
						std::lock_guard lock(mutex);
						// do not check the contents here because discord can change compression
						if (callback.status == 200 && check_mimetype(callback.headers, "image/png")) {
							files_success[2] = true;
						}
						set_file_tested(2);
					});
				}
				else {
					set_file_tested(0);
					set_file_tested(1);
					set_file_tested(2);
				}
			}

			void test_pin() {
				if (!extended) {
					set_pin_tested();
					return;
				}
				set_test(MESSAGEPIN, false);
				set_test(MESSAGEUNPIN, false);
				bot.message_pin(channel_id, message_id, [this](const dpp::confirmation_callback_t &callback) {
					std::lock_guard lock(mutex);
					if (!callback.is_error()) {
						set_test(MESSAGEPIN, true);
						bot.message_unpin(TEST_TEXT_CHANNEL_ID, message_id, [this](const dpp::confirmation_callback_t &callback) {
							std::lock_guard lock(mutex);
							if (!callback.is_error()) {
								set_test(MESSAGEUNPIN, true);
							}
							set_pin_tested();
						});
					}
					else {
						set_pin_tested();
					}
				});
			}

		public:
			dpp::snowflake thread_id;

			explicit message_test_helper(dpp::cluster &_bot) : bot(_bot) {}

			void run(const dpp::message &message) {
				pin_tested = false;
				thread_tested = false;
				files_tested = {false, false, false};
				files_success = {false, false, false};
				channel_id = message.channel_id;
				message_id = message.id;
				test_pin();
				test_files(message);
				test_threads(message);
			}
		};

		message_test_helper message_helper(bot);

		class thread_test_helper {
		public:
			enum event_flag {
				MESSAGE_CREATE = 1 << 0,
				MESSAGE_EDIT = 1 << 1,
				MESSAGE_REACT = 1 << 2,
				MESSAGE_REMOVE_REACT = 1 << 3,
				MESSAGE_DELETE = 1 << 4,
				EVENT_END = 1 << 5
			};
		private:
			std::mutex mutex;
			dpp::cluster &bot;
			bool edit_tested = false;
			bool members_tested = false;
			bool messages_tested = false;
			bool events_tested = false;
			bool get_active_tested = false;
			uint32_t events_tested_mask = 0;
			uint32_t events_to_test_mask = 0;

			void delete_if_done() {
				if (edit_tested && members_tested && messages_tested && events_tested && get_active_tested) {
					bot.channel_delete(thread_id);
				}
			}

			void set_events_tested() {
				if (events_tested) {
					return;
				}
				events_tested = true;
				delete_if_done();
			}

			void set_edit_tested() {
				if (edit_tested) {
					return;
				}
				edit_tested = true;
				delete_if_done();
			}

			void set_members_tested() {
				if (members_tested) {
					return;
				}
				members_tested = true;
				delete_if_done();
			}

			void set_get_active_tested() {
				if (get_active_tested) {
					return;
				}
				get_active_tested = true;
				delete_if_done();
			}

			void set_messages_tested() {
				if (messages_tested) {
					return;
				}
				messages_tested = true;
				delete_if_done();
			}

			void set_event_tested(event_flag flag) {
				if (events_tested_mask & flag) {
					return;
				}
				events_tested_mask |= flag;
				for (uint32_t i = 1; i < EVENT_END; i <<= 1) {
					if ((events_to_test_mask & i) && (events_tested_mask & i) != i) {
						return;
					}
				}
				set_events_tested();
			}

			void events_abort() {
				events_tested_mask |= ~events_to_test_mask;
				for (uint32_t i = 1; i < EVENT_END; i <<= 1) {
					if ((events_tested_mask & i) != i) {
						return;
					}
				}
				set_events_tested();
			}

		public:
			/**
			 * @Brief wrapper for set_event_tested, locking the mutex. Meant to be used from outside the class
			 */
			void notify_event_tested(event_flag flag) {
				std::lock_guard lock{mutex};

				set_event_tested(flag);
			}

			dpp::snowflake thread_id;

			void test_edit(const dpp::thread &thread) {
				std::lock_guard lock{mutex};

				if (!edit_tested) {
					dpp::thread edit = thread;
					set_test(THREAD_EDIT, false);
					set_test(THREAD_UPDATE_EVENT, false);
					edit.name = "edited";
					edit.metadata.locked = true;
					bot.thread_edit(edit, [this](const dpp::confirmation_callback_t &callback) {
						std::lock_guard lock(mutex);
						if (!callback.is_error()) {
							set_test(THREAD_EDIT, true);
						}
						set_edit_tested();
					});
				}
			}

			void test_get_active(const dpp::thread &thread) {
				std::lock_guard lock{mutex};

				set_test(THREAD_GET_ACTIVE, false);
				bot.threads_get_active(TEST_GUILD_ID, [this](const dpp::confirmation_callback_t &callback) {
					std::lock_guard lock{mutex};
					if (!callback.is_error()) {
						const auto &threads = callback.get<dpp::active_threads>();
						if (auto thread_it = threads.find(thread_id); thread_it != threads.end()) {
							const auto &thread = thread_it->second.active_thread;
							const auto &member = thread_it->second.bot_member;
							if (thread.id == thread_id && member.has_value() && member->user_id == bot.me.id) {
								set_test(THREAD_GET_ACTIVE, true);
							}
						}
					}
					set_get_active_tested();
				});
			}

			void test_members(const dpp::thread &thread) {
				std::lock_guard lock{mutex};

				if (!members_tested) {
					if (!extended) {
						set_members_tested();
						return;
					}
					set_test(THREAD_MEMBER_ADD, false);
					set_test(THREAD_MEMBER_GET, false);
					set_test(THREAD_MEMBERS_GET, false);
					set_test(THREAD_MEMBER_REMOVE, false);
					set_test(THREAD_MEMBERS_ADD_EVENT, false);
					set_test(THREAD_MEMBERS_REMOVE_EVENT, false);
					bot.thread_member_add(thread_id, TEST_USER_ID, [this](const dpp::confirmation_callback_t &callback) {
						std::lock_guard lock{mutex};
						if (callback.is_error()) {
							set_members_tested();
							return;
						}
						set_test(THREAD_MEMBER_ADD, true);
						bot.thread_member_get(thread_id, TEST_USER_ID, [this](const dpp::confirmation_callback_t &callback) {
							std::lock_guard lock{mutex};
							if (callback.is_error()) {
								set_members_tested();
								return;
							}
							set_test(THREAD_MEMBER_GET, true);
							bot.thread_members_get(thread_id, [this](const dpp::confirmation_callback_t &callback) {
								std::lock_guard lock{mutex};
								if (callback.is_error()) {
									set_members_tested();
									return;
								}
								const auto &members = callback.get<dpp::thread_member_map>();
								if (members.find(TEST_USER_ID) == members.end() || members.find(bot.me.id) == members.end()) {
									set_members_tested();
									return;
								}
								set_test(THREAD_MEMBERS_GET, true);
								bot.thread_member_remove(thread_id, TEST_USER_ID, [this](const dpp::confirmation_callback_t &callback) {
									std::lock_guard lock{mutex};
									if (!callback.is_error()) {
										set_test(THREAD_MEMBER_REMOVE, true);
									}
									set_members_tested();
								});
							});
						});
					});
				}
			}

			void test_messages(const dpp::thread &thread) {
				if (!extended) {
					set_messages_tested();
					set_events_tested();
					return;
				}

				std::lock_guard lock{mutex};
				set_test(THREAD_MESSAGE, false);
				set_test(THREAD_MESSAGE_CREATE_EVENT, false);
				set_test(THREAD_MESSAGE_EDIT_EVENT, false);
				set_test(THREAD_MESSAGE_REACT_ADD_EVENT, false);
				set_test(THREAD_MESSAGE_REACT_REMOVE_EVENT, false);
				set_test(THREAD_MESSAGE_DELETE_EVENT, false);
				events_to_test_mask |= MESSAGE_CREATE;
				bot.message_create(dpp::message{"hello thread"}.set_channel_id(thread.id), [this](const dpp::confirmation_callback_t &callback) {
					std::lock_guard lock{mutex};
					if (callback.is_error()) {
						events_abort();
						set_messages_tested();
						return;
					}
					auto m = callback.get<dpp::message>();
					m.content = "hello thread?";
					events_to_test_mask |= MESSAGE_EDIT;
					bot.message_edit(m, [this, message_id = m.id](const dpp::confirmation_callback_t &callback) {
						std::lock_guard lock{mutex};
						if (callback.is_error()) {
							events_abort();
							set_messages_tested();
							return;
						}
						events_to_test_mask |= MESSAGE_REACT;
						bot.message_add_reaction(message_id, thread_id, dpp::unicode_emoji::thread, [this, message_id](const dpp::confirmation_callback_t &callback) {
							std::lock_guard lock{mutex};
							if (callback.is_error()) {
								events_abort();
								set_messages_tested();
								return;
							}
							events_to_test_mask |= MESSAGE_REMOVE_REACT;
							bot.message_delete_reaction(message_id, thread_id, bot.me.id, dpp::unicode_emoji::thread, [this, message_id](const dpp::confirmation_callback_t &callback) {
								std::lock_guard lock{mutex};
								if (callback.is_error()) {
									events_abort();
									set_messages_tested();
									return;
								}
								events_to_test_mask |= MESSAGE_DELETE;
								bot.message_delete(message_id, thread_id, [this] (const dpp::confirmation_callback_t &callback) {
								std::lock_guard lock{mutex};
								set_messages_tested();
								if (callback.is_error()) {
									events_abort();
									return;
								}
								set_test(THREAD_MESSAGE, true);
								});
							});
						});
					});
				});
			}

			void run(const dpp::thread &thread) {
				thread_id = thread.id;
				test_get_active(thread);
				test_edit(thread);
				test_members(thread);
				test_messages(thread);
			}

			explicit thread_test_helper(dpp::cluster &bot_) : bot{bot_}
			{
			}
		};

		thread_test_helper thread_helper(bot);

		bot.on_thread_create([&](const dpp::thread_create_t &event) {
			if (event.created.name == "thread test") {
				set_test(THREAD_CREATE_EVENT, true);
				thread_helper.run(event.created);
			}
		});

		bool message_tested = false;
		bot.on_message_create([&](const dpp::message_create_t & event) {
			if (event.msg.author.id == bot.me.id) {
				if (event.msg.content == "test message" && !message_tested) {
					message_tested = true;
					set_test(MESSAGERECEIVE, true);
					message_helper.run(event.msg);
					set_test(MESSAGESGET, false);
					bot.messages_get(event.msg.channel_id, 0, event.msg.id, 0, 5, [](const dpp::confirmation_callback_t &cc){
						if (!cc.is_error()) {
							dpp::message_map mm = std::get<dpp::message_map>(cc.value);
							if (mm.size()) {
								set_test(MESSAGESGET, true);
								set_test(TIMESTAMP, false);
								dpp::message m = mm.begin()->second;
								if (m.sent > 0) {
									set_test(TIMESTAMP, true);
								} else {
									set_test(TIMESTAMP, false);
								}
							} else {
								set_test(MESSAGESGET, false);
							}
						}  else {
							set_test(MESSAGESGET, false);
						}
					});
					set_test(MSGCREATESEND, false);
					event.send("MSGCREATESEND", [&bot, ch_id = event.msg.channel_id] (const auto& cc) {
						if (!cc.is_error()) {
							dpp::message m = std::get<dpp::message>(cc.value);
							if (m.channel_id == ch_id) {
								set_test(MSGCREATESEND, true);
							} else {
								bot.log(dpp::ll_debug, cc.http_info.body);
								set_test(MSGCREATESEND, false);
							}
							bot.message_delete(m.id, m.channel_id);
						} else {
							bot.log(dpp::ll_debug, cc.http_info.body);
							set_test(MSGCREATESEND, false);
						}
					});
				}
				if (event.msg.channel_id == thread_helper.thread_id && event.msg.content == "hello thread") {
					set_test(THREAD_MESSAGE_CREATE_EVENT, true);
					thread_helper.notify_event_tested(thread_test_helper::MESSAGE_CREATE);
				}
			}
		});

		bot.on_message_reaction_add([&](const dpp::message_reaction_add_t & event) {
			if (event.reacting_user.id == bot.me.id) {
				if (event.reacting_emoji.name == "😄") {
					set_test(REACTEVENT, true);
				}
				if (event.channel_id == thread_helper.thread_id && event.reacting_emoji.name == dpp::unicode_emoji::thread) {
					set_test(THREAD_MESSAGE_REACT_ADD_EVENT, true);
					thread_helper.notify_event_tested(thread_test_helper::MESSAGE_REACT);
				}
			}
		});

		bot.on_message_reaction_remove([&](const dpp::message_reaction_remove_t & event) {
			if (event.reacting_user_id == bot.me.id) {
				if (event.channel_id == thread_helper.thread_id && event.reacting_emoji.name == dpp::unicode_emoji::thread) {
					set_test(THREAD_MESSAGE_REACT_REMOVE_EVENT, true);
					thread_helper.notify_event_tested(thread_test_helper::MESSAGE_REMOVE_REACT);
				}
			}
		});

		bot.on_message_delete([&](const dpp::message_delete_t & event) {
			if (event.channel_id == thread_helper.thread_id) {
				set_test(THREAD_MESSAGE_DELETE_EVENT, true);
				thread_helper.notify_event_tested(thread_test_helper::MESSAGE_DELETE);
			}
		});

		bool message_edit_tested = false;
		bot.on_message_update([&](const dpp::message_update_t &event) {
			if (event.msg.author == bot.me.id) {
				if (event.msg.content == "test edit" && !message_edit_tested) {
					message_edit_tested = true;
					set_test(EDITEVENT, true);
				}
				if (event.msg.channel_id == thread_helper.thread_id && event.msg.content == "hello thread?") {
					set_test(THREAD_MESSAGE_EDIT_EVENT, true);
					thread_helper.notify_event_tested(thread_test_helper::MESSAGE_EDIT);
				}
			}
		});

		bot.on_thread_update([&](const dpp::thread_update_t &event) {
			if (event.updating_guild->id == TEST_GUILD_ID && event.updated.id == thread_helper.thread_id && event.updated.name == "edited") {
				set_test(THREAD_UPDATE_EVENT, true);
			}
		});

		bot.on_thread_members_update([&](const dpp::thread_members_update_t &event) {
			if (event.updating_guild->id == TEST_GUILD_ID && event.thread_id == thread_helper.thread_id) {
				if (std::find_if(std::begin(event.added), std::end(event.added), is_owner) != std::end(event.added)) {
					set_test(THREAD_MEMBERS_ADD_EVENT, true);
				}
				if (std::find_if(std::begin(event.removed_ids), std::end(event.removed_ids), is_owner) != std::end(event.removed_ids)) {
					set_test(THREAD_MEMBERS_REMOVE_EVENT, true);
				}
			}
		});

		bot.on_thread_delete([&](const dpp::thread_delete_t &event) {
			if (event.deleting_guild->id == TEST_GUILD_ID && event.deleted.id == message_helper.thread_id) {
				set_test(THREAD_DELETE_EVENT, true);
			}
		});

		// set to execute from this thread (main thread) after on_ready is fired
		auto do_online_tests = [&] {
			coro_online_tests(&bot);
			set_test(GUILD_BAN_CREATE, false);
			set_test(GUILD_BAN_GET, false);
			set_test(GUILD_BANS_GET, false);
			set_test(GUILD_BAN_DELETE, false);
			if (!offline) {
				// some deleted discord accounts to test the ban stuff with...
				dpp::snowflake deadUser1(802670069523415057);
				dpp::snowflake deadUser2(875302419335094292);
				dpp::snowflake deadUser3(1048247361903792198);

				bot.set_audit_reason("ban reason one").guild_ban_add(TEST_GUILD_ID, deadUser1, 0, [deadUser1, deadUser2, deadUser3, &bot](const dpp::confirmation_callback_t &event) {
				if (!event.is_error()) bot.guild_ban_add(TEST_GUILD_ID, deadUser2, 0, [deadUser1, deadUser2, deadUser3, &bot](const dpp::confirmation_callback_t &event) {
						if (!event.is_error()) bot.set_audit_reason("ban reason three").guild_ban_add(TEST_GUILD_ID, deadUser3, 0, [deadUser1, deadUser2, deadUser3, &bot](const dpp::confirmation_callback_t &event) {
							if (event.is_error()) {
								return;
							}
							set_test(GUILD_BAN_CREATE, true);
							// when created, continue with getting and deleting

							// get ban
							bot.guild_get_ban(TEST_GUILD_ID, deadUser1, [deadUser1](const dpp::confirmation_callback_t &event) {
								if (!event.is_error()) {
									dpp::ban ban = event.get<dpp::ban>();
									if (ban.user_id == deadUser1 && ban.reason == "ban reason one") {
										set_test(GUILD_BAN_GET, true);
									}
								}
							});

							// get multiple bans
							bot.guild_get_bans(TEST_GUILD_ID, 0, deadUser1, 3, [deadUser2, deadUser3](const dpp::confirmation_callback_t &event) {
								if (!event.is_error()) {
									dpp::ban_map bans = event.get<dpp::ban_map>();
									int successCount = 0;
									for (auto &ban: bans) {
										if (ban.first == ban.second.user_id) { // the key should match the ban's user_id
											if (ban.first == deadUser2 && ban.second.reason.empty()) {
												successCount++;
											} else if (ban.first == deadUser3 && ban.second.reason == "ban reason three") {
												successCount++;
											}
										}
									}
									if (successCount == 2) {
										set_test(GUILD_BANS_GET, true);
									}
								}
							});

							// unban them
							bot.guild_ban_delete(TEST_GUILD_ID, deadUser1, [&bot, deadUser2, deadUser3](const dpp::confirmation_callback_t &event) {
								if (!event.is_error()) {
									bot.guild_ban_delete(TEST_GUILD_ID, deadUser2, [&bot, deadUser3](const dpp::confirmation_callback_t &event) {
										if (!event.is_error()) {
											bot.guild_ban_delete(TEST_GUILD_ID, deadUser3, [](const dpp::confirmation_callback_t &event) {
												if (!event.is_error()) {
													set_test(GUILD_BAN_DELETE, true);
												}
											});
										}
									});
								}
							});
						});
					});
				});
			}

			set_test(REQUEST_GET_IMAGE, false);
			if (!offline) {
				bot.request("https://dpp.dev/DPP-Logo.png", dpp::m_get, [&bot](const dpp::http_request_completion_t &callback) {
					if (callback.status != 200) {
						return;
					}
					set_test(REQUEST_GET_IMAGE, true);

					dpp::emoji emoji;
					emoji.load_image(callback.body, dpp::i_png);
					emoji.name = "dpp";

					// emoji unit test with the requested image
					set_test(EMOJI_CREATE, false);
					set_test(EMOJI_GET, false);
					set_test(EMOJI_DELETE, false);
					bot.guild_emoji_create(TEST_GUILD_ID, emoji, [&bot](const dpp::confirmation_callback_t &event) {
						if (event.is_error()) {
							return;
						}
						set_test(EMOJI_CREATE, true);

						auto created = event.get<dpp::emoji>();
						bot.guild_emoji_get(TEST_GUILD_ID, created.id, [&bot, created](const dpp::confirmation_callback_t &event) {
							if (event.is_error()) {
								return;
							}
							auto fetched = event.get<dpp::emoji>();
							if (created.id == fetched.id && created.name == fetched.name && created.flags == fetched.flags) {
								set_test(EMOJI_GET, true);
							}

							bot.guild_emoji_delete(TEST_GUILD_ID, fetched.id, [](const dpp::confirmation_callback_t &event) {
								if (!event.is_error()) {
									set_test(EMOJI_DELETE, true);
								}
							});
						});
					});
				});
			}

			set_test(INVITE_CREATE, false);
			set_test(INVITE_GET, false);
			set_test(INVITE_DELETE, false);
			if (!offline) {
				dpp::channel channel;
				channel.id = TEST_TEXT_CHANNEL_ID;
				dpp::invite invite;
				invite.max_age = 0;
				invite.max_uses = 100;
				set_test(INVITE_CREATE_EVENT, false);
				bot.channel_invite_create(channel, invite, [&bot, invite](const dpp::confirmation_callback_t &event) {
					if (event.is_error()) {
						return;
					}

					auto created = event.get<dpp::invite>();
					if (!created.code.empty() && created.channel_id == TEST_TEXT_CHANNEL_ID && created.guild_id == TEST_GUILD_ID && created.inviter.id == bot.me.id) {
						set_test(INVITE_CREATE, true);
					}

					bot.invite_get(created.code, [&bot, created](const dpp::confirmation_callback_t &event) {
						if (!event.is_error()) {
							auto retrieved = event.get<dpp::invite>();
							if (retrieved.code == created.code && retrieved.guild_id == created.guild_id && retrieved.channel_id == created.channel_id && retrieved.inviter.id == created.inviter.id) {
								if (retrieved.destination_guild.flags & dpp::g_community) {
									set_test(INVITE_GET, retrieved.expires_at == 0);
								} else {
									set_test(INVITE_GET, true);
								}

							} else {
								set_test(INVITE_GET, false);
							}
						} else {
							set_test(INVITE_GET, false);
						}

						set_test(INVITE_DELETE_EVENT, false);
						bot.invite_delete(created.code, [](const dpp::confirmation_callback_t &event) {
						set_test(INVITE_DELETE, !event.is_error());
						});
					});
				});
			}

			set_test(AUTOMOD_RULE_CREATE, false);
			set_test(AUTOMOD_RULE_GET, false);
			set_test(AUTOMOD_RULE_GET_ALL, false);
			set_test(AUTOMOD_RULE_DELETE, false);
			if (!offline) {
				dpp::automod_rule automodRule;
				automodRule.name = "automod rule (keyword type)";
				automodRule.trigger_type = dpp::amod_type_keyword;
				dpp::automod_metadata metadata1;
				metadata1.keywords.emplace_back("*cat*");
				metadata1.keywords.emplace_back("train");
				metadata1.keywords.emplace_back("*.exe");
				metadata1.regex_patterns.emplace_back("^[^a-z]$");
				metadata1.allow_list.emplace_back("@silent*");
				automodRule.trigger_metadata = metadata1;
				dpp::automod_action automodAction;
				automodAction.type = dpp::amod_action_timeout;
				automodAction.duration_seconds = 6000;
				automodRule.actions.emplace_back(automodAction);

				bot.automod_rules_get(TEST_GUILD_ID, [&bot, automodRule](const dpp::confirmation_callback_t &event) {
					if (event.is_error()) {
						return;
					}
					auto rules = event.get<dpp::automod_rule_map>();
					set_test(AUTOMOD_RULE_GET_ALL, true);
					for (const auto &rule: rules) {
						if (rule.second.trigger_type == dpp::amod_type_keyword) {
							// delete one automod rule of type KEYWORD before creating one to make space...
							bot.automod_rule_delete(TEST_GUILD_ID, rule.first);
						}
					}

					// start creating the automod rules
					bot.automod_rule_create(TEST_GUILD_ID, automodRule, [&bot, automodRule](const dpp::confirmation_callback_t &event) {
						if (event.is_error()) {
							return;
						}
						auto created = event.get<dpp::automod_rule>();
						if (created.name == automodRule.name) {
							set_test(AUTOMOD_RULE_CREATE, true);
						}

						// get automod rule
						bot.automod_rule_get(TEST_GUILD_ID, created.id, [automodRule, &bot, created](const dpp::confirmation_callback_t &event) {
							if (event.is_error()) {
								return;
							}
							auto retrieved = event.get<dpp::automod_rule>();
							if (retrieved.name == automodRule.name &&
								retrieved.trigger_type == automodRule.trigger_type &&
								retrieved.trigger_metadata.keywords == automodRule.trigger_metadata.keywords &&
								retrieved.trigger_metadata.regex_patterns == automodRule.trigger_metadata.regex_patterns &&
								retrieved.trigger_metadata.allow_list == automodRule.trigger_metadata.allow_list && retrieved.actions.size() == automodRule.actions.size()) {
								set_test(AUTOMOD_RULE_GET, true);
							}

							// delete the automod rule
							bot.automod_rule_delete(TEST_GUILD_ID, retrieved.id, [](const dpp::confirmation_callback_t &event) {
								if (!event.is_error()) {
									set_test(AUTOMOD_RULE_DELETE, true);
								}
							});
						});
					});
				});
			}

			set_test(USER_GET, false);
			set_test(USER_GET_FLAGS, false);
			if (!offline) {
				bot.user_get(TEST_USER_ID, [](const dpp::confirmation_callback_t &event) {
					if (!event.is_error()) {
						auto u = std::get<dpp::user_identified>(event.value);
						if (u.id == TEST_USER_ID) {
							set_test(USER_GET, true);
						} else {
							set_test(USER_GET, false);
						}
						json j = json::parse(event.http_info.body);
						uint64_t raw_flags = j["public_flags"];
						if (j.contains("flags")) {
							uint64_t flags = j["flags"];
							raw_flags |= flags;
						}
						// testing all user flags from https://discord.com/developers/docs/resources/user#user-object-user-flags
						// they're manually set here because the dpp::user_flags don't match to the discord API, so we can't use them to compare with the raw flags!
						if (
							u.is_discord_employee() == 			((raw_flags & (1 << 0)) != 0) &&
							u.is_partnered_owner() == 			((raw_flags & (1 << 1)) != 0) &&
							u.has_hypesquad_events() == 		((raw_flags & (1 << 2)) != 0) &&
							u.is_bughunter_1() == 				((raw_flags & (1 << 3)) != 0) &&
							u.is_house_bravery() == 			((raw_flags & (1 << 6)) != 0) &&
							u.is_house_brilliance() == 			((raw_flags & (1 << 7)) != 0) &&
							u.is_house_balance() == 			((raw_flags & (1 << 8)) != 0) &&
							u.is_early_supporter() == 			((raw_flags & (1 << 9)) != 0) &&
							u.is_team_user() == 				((raw_flags & (1 << 10)) != 0) &&
							u.is_bughunter_2() == 				((raw_flags & (1 << 14)) != 0) &&
							u.is_verified_bot() == 				((raw_flags & (1 << 16)) != 0) &&
							u.is_verified_bot_dev() == 			((raw_flags & (1 << 17)) != 0) &&
							u.is_certified_moderator() == 		((raw_flags & (1 << 18)) != 0) &&
							u.is_bot_http_interactions() == 	((raw_flags & (1 << 19)) != 0) &&
							u.is_active_developer() == 			((raw_flags & (1 << 22)) != 0)
							) {
							set_test(USER_GET_FLAGS, true);
						} else {
							set_test(USER_GET_FLAGS, false);
						}
					} else {
						set_test(USER_GET, false);
						set_test(USER_GET_FLAGS, false);
					}
				});
			}

			set_test(VOICE_CHANNEL_CREATE, false);
			set_test(VOICE_CHANNEL_EDIT, false);
			set_test(VOICE_CHANNEL_DELETE, false);
			if (!offline) {
				dpp::channel channel1;
				channel1.set_type(dpp::CHANNEL_VOICE)
					.set_guild_id(TEST_GUILD_ID)
					.set_name("voice1")
					.add_permission_overwrite(TEST_GUILD_ID, dpp::ot_role, 0, dpp::p_view_channel)
					.set_user_limit(99);
				dpp::channel createdChannel;
				try {
					createdChannel = bot.channel_create_sync(channel1);
				} catch (dpp::rest_exception &exception) {
					set_test(VOICE_CHANNEL_CREATE, false);
				}
				if (createdChannel.name == channel1.name &&
				createdChannel.user_limit == 99 &&
				createdChannel.name == "voice1") {
					for (auto overwrite: createdChannel.permission_overwrites) {
						if (overwrite.id == TEST_GUILD_ID && overwrite.type == dpp::ot_role && overwrite.deny == dpp::p_view_channel) {
							set_test(VOICE_CHANNEL_CREATE, true);
						}
					}

					// edit the voice channel
					createdChannel.set_name("foobar2");
					createdChannel.set_user_limit(2);
					for (auto overwrite: createdChannel.permission_overwrites) {
						if (overwrite.id == TEST_GUILD_ID) {
							overwrite.deny.set(0);
							overwrite.allow.set(dpp::p_view_channel);
						}
					}
					try {
						dpp::channel edited = bot.channel_edit_sync(createdChannel);
						if (edited.name == "foobar2" && edited.user_limit == 2) {
							set_test(VOICE_CHANNEL_EDIT, true);
						}
					} catch (dpp::rest_exception &exception) {
						set_test(VOICE_CHANNEL_EDIT, false);
					}

					// delete the voice channel
					try {
						bot.channel_delete_sync(createdChannel.id);
						set_test(VOICE_CHANNEL_DELETE, true);
					} catch (dpp::rest_exception &exception) {
						set_test(VOICE_CHANNEL_DELETE, false);
					}
				}
			}

			set_test(FORUM_CREATION, false);
			set_test(FORUM_CHANNEL_GET, false);
			set_test(FORUM_CHANNEL_DELETE, false);
			if (!offline) {
				dpp::channel c;
				c.name = "test-forum-channel";
				c.guild_id = TEST_GUILD_ID;
				c.set_topic("This is a forum channel");
				c.set_flags(dpp::CHANNEL_FORUM);
				c.default_sort_order = dpp::so_creation_date;
				dpp::forum_tag t;
				t.name = "Alpha";
				t.emoji = "❌";
				c.available_tags = {t};
				c.default_auto_archive_duration = dpp::arc_1_day;
				c.default_reaction = "✅";
				c.default_thread_rate_limit_per_user = 10;
				bot.channel_create(c, [&bot](const dpp::confirmation_callback_t &event) {
					if (!event.is_error()) {
						set_test(FORUM_CREATION, true);
						auto channel = std::get<dpp::channel>(event.value);
						// retrieve the forum channel and check the values
						bot.channel_get(channel.id, [forum_id = channel.id, &bot](const dpp::confirmation_callback_t &event) {
							if (!event.is_error()) {
								auto channel = std::get<dpp::channel>(event.value);
								bot.log(dpp::ll_debug, event.http_info.body);
								bool tag = false;
								for (auto &t : channel.available_tags) {
									if (t.name == "Alpha" && std::holds_alternative<std::string>(t.emoji) && std::get<std::string>(t.emoji) == "❌") {
										tag = true;
									}
								}
								bool name = channel.name == "test-forum-channel";
								bool sort = channel.default_sort_order == dpp::so_creation_date;
								bool rateLimit = channel.default_thread_rate_limit_per_user == 10;
								set_test(FORUM_CHANNEL_GET, tag && name && sort && rateLimit);
							} else {
								set_test(FORUM_CHANNEL_GET, false);
							}
							// delete the forum channel
							bot.channel_delete(forum_id, [](const dpp::confirmation_callback_t &event) {
								if (!event.is_error()) {
									set_test(FORUM_CHANNEL_DELETE, true);
								} else {
									set_test(FORUM_CHANNEL_DELETE, false);
								}
							});
						});
					} else {
						set_test(FORUM_CREATION, false);
						set_test(FORUM_CHANNEL_GET, false);
					}
				});
			}

			set_test(THREAD_CREATE, false);
			if (!offline) {
				bot.thread_create("thread test", TEST_TEXT_CHANNEL_ID, 60, dpp::channel_type::CHANNEL_PUBLIC_THREAD, true, 60, [&](const dpp::confirmation_callback_t &event) {
					if (!event.is_error()) {
						[[maybe_unused]] const auto &thread = event.get<dpp::thread>();
						set_test(THREAD_CREATE, true);
					}
					// the thread tests are in the on_thread_create event handler
				});
			}

			set_test(MEMBER_GET, false);
			if (!offline) {
				bot.guild_get_member(TEST_GUILD_ID, TEST_USER_ID, [](const dpp::confirmation_callback_t &event){
					if (!event.is_error()) {
						dpp::guild_member m = std::get<dpp::guild_member>(event.value);
						if (m.guild_id == TEST_GUILD_ID && m.user_id == TEST_USER_ID) {
							set_test(MEMBER_GET, true);
						} else {
							set_test(MEMBER_GET, false);
						}
					} else {
						set_test(MEMBER_GET, false);
					}
				});
			}

			set_test(ROLE_CREATE, false);
			set_test(ROLE_EDIT, false);
			set_test(ROLE_DELETE, false);
			if (!offline) {
				dpp::role r;
				r.guild_id = TEST_GUILD_ID;
				r.name = "Test-Role";
				r.permissions.add(dpp::p_move_members);
				r.set_flags(dpp::r_mentionable);
				r.colour = dpp::colors::moon_yellow;
				dpp::role createdRole;
				try {
					createdRole = bot.role_create_sync(r);
					if (createdRole.name == r.name &&
					createdRole.has_move_members() &&
					createdRole.flags & dpp::r_mentionable &&
					createdRole.colour == r.colour) {
						set_test(ROLE_CREATE, true);
					}
				} catch (dpp::rest_exception &exception) {
					set_test(ROLE_CREATE, false);
				}
				createdRole.guild_id = TEST_GUILD_ID;
				createdRole.name = "Test-Role-Edited";
				createdRole.colour = dpp::colors::light_sea_green;
				try {
					dpp::role edited = bot.role_edit_sync(createdRole);
					if (createdRole.id == edited.id && edited.name == "Test-Role-Edited") {
						set_test(ROLE_EDIT, true);
					}
				} catch (dpp::rest_exception &exception) {
					set_test(ROLE_EDIT, false);
				}
				try {
					bot.role_delete_sync(TEST_GUILD_ID, createdRole.id);
					set_test(ROLE_DELETE, true);
				} catch (dpp::rest_exception &exception) {
					set_test(ROLE_DELETE, false);
				}
			}
		};

		set_test(BOTSTART, false);
		try {
			if (!offline) {
				bot.start(true);
				set_test(BOTSTART, true);
			}
		}
		catch (const std::exception &) {
			set_test(BOTSTART, false);
		}

		set_test(TIMERSTART, false);
		uint32_t ticks = 0;
		dpp::timer th = bot.start_timer([&](dpp::timer timer_handle) {
			if (ticks == 5) {
				/* The simple test timer ticks every second.
				 * If we get to 5 seconds, we know the timer is working.
				 */
				set_test(TIMERSTART, true);
			}
			ticks++;
		}, 1);

		set_test(TIMEDLISTENER, false);
		dpp::timed_listener tl(&bot, 10, bot.on_log, [&](const dpp::log_t & event) {
			set_test(TIMEDLISTENER, true);
		});

		set_test(ONESHOT, false);
		bool once = false;
		dpp::oneshot_timer ost(&bot, 5, [&](dpp::timer timer_handle) {
			if (!once) {
				set_test(ONESHOT, true);
			} else {
				set_test(ONESHOT, false);
			}
			once = true;
		});

		// online tests
		if (!offline) {
			if (std::future_status status = ready_future.wait_for(std::chrono::seconds(20)); status != std::future_status::timeout) {
				do_online_tests();
			}
		}

		noparam_api_test(current_user_get, dpp::user_identified, CURRENTUSER);
		singleparam_api_test(channel_get, TEST_TEXT_CHANNEL_ID, dpp::channel, GETCHAN);
		singleparam_api_test(guild_get, TEST_GUILD_ID, dpp::guild, GETGUILD);
		singleparam_api_test_list(roles_get, TEST_GUILD_ID, dpp::role_map, GETROLES);
		singleparam_api_test_list(channels_get, TEST_GUILD_ID, dpp::channel_map, GETCHANS);
		singleparam_api_test_list(guild_get_invites, TEST_GUILD_ID, dpp::invite_map, GETINVS);
		multiparam_api_test_list(guild_get_bans, TEST_GUILD_ID, dpp::ban_map, GETBANS);
		singleparam_api_test_list(channel_pins_get, TEST_TEXT_CHANNEL_ID, dpp::message_map, GETPINS);
		singleparam_api_test_list(guild_events_get, TEST_GUILD_ID, dpp::scheduled_event_map, GETEVENTS);
		twoparam_api_test(guild_event_get, TEST_GUILD_ID, TEST_EVENT_ID, dpp::scheduled_event, GETEVENT);
		twoparam_api_test_list(guild_event_users_get, TEST_GUILD_ID, TEST_EVENT_ID, dpp::event_member_map, GETEVENTUSERS);

		std::this_thread::sleep_for(std::chrono::seconds(20));

		/* Test stopping timer */
		set_test(TIMERSTOP, false);
		set_test(TIMERSTOP, bot.stop_timer(th));

		set_test(USERCACHE, false);
		if (!offline) {
			dpp::user* u = dpp::find_user(TEST_USER_ID);
			set_test(USERCACHE, u);
		}
		set_test(CHANNELCACHE, false);
		set_test(CHANNELTYPES, false);
		if (!offline) {
			dpp::channel* c = dpp::find_channel(TEST_TEXT_CHANNEL_ID);
			dpp::channel* c2 = dpp::find_channel(TEST_VC_ID);
			set_test(CHANNELCACHE, c && c2);
			set_test(CHANNELTYPES, c && c->is_text_channel() && !c->is_voice_channel() && c2 && c2->is_voice_channel() && !c2->is_text_channel());
		}

		wait_for_tests();

	}
	catch (const std::exception &e) {
		std::cout << e.what() << "\n";
		set_test(CLUSTER, false);
	}
}
