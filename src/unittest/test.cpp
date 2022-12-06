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
#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

/* Unit tests go here */
int main()
{
	std::string token(get_token());

	std::cout << "[" << std::fixed << std::setprecision(3) << (dpp::utility::time_f() - get_start_time()) << "]: [\u001b[36mSTART\u001b[0m] ";
	if (offline) {
		std::cout << "Running offline unit tests only.\n";
	} else {
		std::cout << "Running offline and online unit tests. Guild ID: " << TEST_GUILD_ID << " Text Channel ID: " << TEST_TEXT_CHANNEL_ID << " VC ID: " << TEST_VC_ID << " User ID: " << TEST_USER_ID << " Event ID: " << TEST_EVENT_ID << "\n";
	}

	std::string test_to_escape = "*** _This is a test_ ***\n```cpp\n\
int main() {\n\
    /* Comment */\n\
    int answer = 42;\n\
    return answer; // ___\n\
};\n\
```\n\
Markdown lol ||spoiler|| ~~strikethrough~~ `small *code* block`\n";

	set_test("COMPARISON", false);
	dpp::user u1;
	dpp::user u2;
	dpp::user u3;
	u1.id = u2.id = 666;
	u3.id = 777;
	set_test("COMPARISON", u1 == u2 && u1 != u3);


	set_test("MD_ESC_1", false);
	set_test("MD_ESC_2", false);
	std::string escaped1 = dpp::utility::markdown_escape(test_to_escape);
	std::string escaped2 = dpp::utility::markdown_escape(test_to_escape, true);
	set_test("MD_ESC_1", escaped1 == "\\*\\*\\* \\_This is a test\\_ \\*\\*\\*\n\
```cpp\n\
int main() {\n\
    /* Comment */\n\
    int answer = 42;\n\
    return answer; // ___\n\
};\n\
```\n\
Markdown lol \\|\\|spoiler\\|\\| \\~\\~strikethrough\\~\\~ `small *code* block`\n");
	set_test("MD_ESC_2", escaped2 == "\\*\\*\\* \\_This is a test\\_ \\*\\*\\*\n\
\\`\\`\\`cpp\n\
int main\\(\\) {\n\
    /\\* Comment \\*/\n\
    int answer = 42;\n\
    return answer; // \\_\\_\\_\n\
};\n\
\\`\\`\\`\n\
Markdown lol \\|\\|spoiler\\|\\| \\~\\~strikethrough\\~\\~ \\`small \\*code\\* block\\`\n");

	set_test("URLENC", false);
	set_test("URLENC", dpp::utility::url_encode("ABC123_+\\|$*/AAA[]😄") == "ABC123_%2B%5C%7C%24%2A%2FAAA%5B%5D%F0%9F%98%84");

	dpp::http_connect_info hci;
	set_test("HOSTINFO", false);

	hci = dpp::https_client::get_host_info("https://test.com:444");
	bool hci_test = (hci.scheme == "https" && hci.hostname == "test.com" && hci.port == 444 && hci.is_ssl == true);

	hci = dpp::https_client::get_host_info("https://test.com");
	hci_test = hci_test && (hci.scheme == "https" && hci.hostname == "test.com" && hci.port == 443 && hci.is_ssl == true);

	hci = dpp::https_client::get_host_info("http://test.com");
	hci_test = hci_test && (hci.scheme == "http" && hci.hostname == "test.com" && hci.port == 80 && hci.is_ssl == false);

	hci = dpp::https_client::get_host_info("http://test.com:90");
	hci_test = hci_test && (hci.scheme == "http" && hci.hostname == "test.com" && hci.port == 90 && hci.is_ssl == false);

	hci = dpp::https_client::get_host_info("test.com:97");
	hci_test = hci_test && (hci.scheme == "http" && hci.hostname == "test.com" && hci.port == 97 && hci.is_ssl == false);

	hci = dpp::https_client::get_host_info("test.com");
	hci_test = hci_test && (hci.scheme == "http" && hci.hostname == "test.com" && hci.port == 80 && hci.is_ssl == false);

	set_test("HOSTINFO", hci_test);

	set_test("HTTPS", false);
	if (!offline) {
		dpp::multipart_content multipart = dpp::https_client::build_multipart(
			"{\"content\":\"test\"}", {"test.txt", "blob.blob"}, {"ABCDEFGHI", "BLOB!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"}
		);
		try {
			dpp::https_client c("discord.com", 443, "/api/channels/" + std::to_string(TEST_TEXT_CHANNEL_ID) + "/messages", "POST", multipart.body,
				{
					{"Content-Type", multipart.mimetype},
					{"Authorization", "Bot " + token}
				}
			);
			std::string hdr1 = c.get_header("server");
			std::string content1 = c.get_content();
			set_test("HTTPS", hdr1 == "cloudflare" && c.get_status() == 200);
		}
		catch (const dpp::exception& e) {
			std::cout << e.what() << "\n";
			set_test("HTTPS", false);
		}
	}

	set_test("HTTP", false);
	try {
		dpp::https_client c2("github.com", 80, "/", "GET", "", {}, true);
		std::string hdr2 = c2.get_header("location");
		std::string content2 = c2.get_content();
		set_test("HTTP", hdr2 == "https://github.com/" && c2.get_status() == 301);
	}
	catch (const dpp::exception& e) {
		std::cout << e.what() << "\n";
		set_test("HTTP", false);
	}

	std::vector<uint8_t> testaudio = load_test_audio();

	set_test("READFILE", false);
	std::string rf_test = dpp::utility::read_file(SHARED_OBJECT);
	FILE* fp = fopen(SHARED_OBJECT, "rb");
	fseek(fp, 0, SEEK_END);
	size_t off = (size_t)ftell(fp);
	fclose(fp);
	set_test("READFILE", off == rf_test.length());

	set_test("TIMESTAMPTOSTRING", false);
#ifndef _WIN32
	set_test("TIMESTAMPTOSTRING", dpp::ts_to_string(1642611864) == "2022-01-19T17:04:24Z");
#else
	set_test("TIMESTAMPTOSTRING", true);
#endif

	set_test("ROLE.COMPARE", false);
	dpp::role role_1, role_2;
	role_1.position = 1;
	role_2.position = 2;
	set_test("ROLE.COMPARE", role_1 < role_2 && role_1 != role_2);

	set_test("WEBHOOK", false);
	try {
		dpp::webhook test_wh("https://discord.com/api/webhooks/833047646548133537/ntCHEYYIoHSLy_GOxPx6pmM0sUoLbP101ct-WI6F-S4beAV2vaIcl_Id5loAMyQwxqhE");
		set_test("WEBHOOK", (test_wh.token == "ntCHEYYIoHSLy_GOxPx6pmM0sUoLbP101ct-WI6F-S4beAV2vaIcl_Id5loAMyQwxqhE") && (test_wh.id == dpp::snowflake(833047646548133537)));
	}
	catch (const dpp::exception&) {
		set_test("WEBHOOK", false);
	}

	{ // test dpp::command_option_choice::fill_from_json
		set_test("OPTCHOICE_DOUBLE", false);
		set_test("OPTCHOICE_INT", false);
		set_test("OPTCHOICE_BOOL", false);
		set_test("OPTCHOICE_SNOWFLAKE", false);
		set_test("OPTCHOICE_STRING", false);
		json j;
		dpp::command_option_choice choice;
		j["value"] = 54.321;
		choice.fill_from_json(&j);
		bool success_double = std::holds_alternative<double>(choice.value);
		j["value"] = 8223372036854775807;
		choice.fill_from_json(&j);
		bool success_int = std::holds_alternative<int64_t>(choice.value);
		j["value"] = -8223372036854775807;
		choice.fill_from_json(&j);
		bool success_int2 = std::holds_alternative<int64_t>(choice.value);
		j["value"] = true;
		choice.fill_from_json(&j);
		bool success_bool = std::holds_alternative<bool>(choice.value);
		dpp::snowflake s(845266178036516757); // example snowflake
		j["value"] = s;
		choice.fill_from_json(&j);
		bool success_snowflake = std::holds_alternative<dpp::snowflake>(choice.value) && std::get<dpp::snowflake>(choice.value) == s;
		j["value"] = "foobar";
		choice.fill_from_json(&j);
		bool success_string = std::holds_alternative<std::string>(choice.value);
		set_test("OPTCHOICE_DOUBLE", success_double);
		set_test("OPTCHOICE_INT", success_int && success_int2);
		set_test("OPTCHOICE_BOOL", success_bool);
		set_test("OPTCHOICE_SNOWFLAKE", success_snowflake);
		set_test("OPTCHOICE_STRING", success_string);
	}

	{
		set_test("PERMISSION_CLASS", false);
		bool success = false;
		auto p = dpp::permission();
		p = 16;
		success = p == 16;
		p |= 4;
		success = p == 20 && success;
		p <<= 8; // left shift
		success = p == 5120 && success;
		auto s = std::to_string(p);
		success = s == "5120" && success;
		json j;
		j["value"] = p;
#ifndef _WIN32
		success = dpp::snowflake_not_null(&j, "value") == 5120 && success;
#endif
		p.set(dpp::p_administrator, dpp::p_ban_members);
		success = p.has(dpp::p_administrator) && success;
		success = p.has(dpp::p_administrator) && p.has(dpp::p_ban_members) && success;
		success = p.has(dpp::p_administrator, dpp::p_ban_members) && success;
		success = p.has(dpp::p_administrator | dpp::p_ban_members) && success;

		p.set(dpp::p_administrator);
		success = ! p.has(dpp::p_administrator, dpp::p_ban_members) && success; // must return false because they're not both set
		success = ! p.has(dpp::p_administrator | dpp::p_ban_members) && success;
		set_test("PERMISSION_CLASS", success);
	}

	{ // some dpp::user methods
		dpp::user user1;
		user1.id = 189759562910400512;
		user1.discriminator = 0001;
		user1.username = "brain";

		set_test("USER.GET_MENTION", false);
		set_test("USER.GET_MENTION", user1.get_mention() == "<@189759562910400512>");

		set_test("USER.FORMAT_USERNAME", false);
		set_test("USER.FORMAT_USERNAME", user1.format_username() == "brain#0001");

		set_test("USER.GET_CREATION_TIME", false);
		set_test("USER.GET_CREATION_TIME", (uint64_t)user1.get_creation_time() == 1465312605);
	}

	{ // channel methods
		set_test("CHANNEL.SET_TYPE", false);
		dpp::channel c;
		c.set_flags(dpp::c_nsfw | dpp::c_video_quality_720p);
		c.set_type(dpp::CHANNEL_CATEGORY);
		bool before = c.is_category() && !c.is_forum();
		c.set_type(dpp::CHANNEL_FORUM);
		bool after = !c.is_category() && c.is_forum();
		set_test("CHANNEL.SET_TYPE", before && after);

		set_test("CHANNEL.GET_MENTION", false);
		c.id = 825411707521728511;
		set_test("CHANNEL.GET_MENTION", c.get_mention() == "<#825411707521728511>");
	}

	{ // utility methods
		set_test("UTILITY.ICONHASH", false);
		auto iconhash1 = dpp::utility::iconhash("a_5532c6414c70765a28cf9448c117205f");
		set_test("UTILITY.ICONHASH", iconhash1.first == 6139187225817019994 &&
									 iconhash1.second == 2940732121894297695 &&
									 iconhash1.to_string() == "5532c6414c70765a28cf9448c117205f"
		);

		set_test("UTILITY.MAKE_URL_PARAMETERS", false);
		auto url_params1 = dpp::utility::make_url_parameters({
			{"foo", 15},
			{"bar", 7}
		});
		auto url_params2 = dpp::utility::make_url_parameters({
			{"foo", "hello"},
			{"bar", "two words"}
		});
		set_test("UTILITY.MAKE_URL_PARAMETERS", url_params1 == "?bar=7&foo=15" && url_params2 == "?bar=two%20words&foo=hello");

		set_test("UTILITY.MARKDOWN_ESCAPE", false);
		auto escaped = dpp::utility::markdown_escape(
				"> this is a quote\n"
				"**some bold text**");
		set_test("UTILITY.MARKDOWN_ESCAPE", "\\>this is a quote\\n\\*\\*some bold text\\*\\*");

		set_test("UTILITY.TOKENIZE", false);
		auto tokens = dpp::utility::tokenize("some Whitespace seperated Text to Tokenize", " ");
		std::vector<std::string> expected_tokens = {"some", "Whitespace", "seperated", "Text", "to", "Tokenize"};
		set_test("UTILITY.TOKENIZE", tokens == expected_tokens);

		set_test("UTILITY.URL_ENCODE", false);
		auto url_encoded = dpp::utility::url_encode("S2-^$1Nd+U!g'8+_??o?p-bla bla");
		set_test("UTILITY.URL_ENCODE", url_encoded == "S2-%5E%241Nd%2BU%21g%278%2B_%3F%3Fo%3Fp-bla%20bla");

		set_test("UTILITY.SLASHCOMMAND_MENTION", false);
		auto mention1 = dpp::utility::slashcommand_mention(123, "name");
		auto mention2 = dpp::utility::slashcommand_mention(123, "name", "sub");
		auto mention3 = dpp::utility::slashcommand_mention(123, "name", "group", "sub");
		bool success = mention1 == "</name:123>" && mention2 == "</name sub:123>" && mention3 == "</name group sub:123>";
		set_test("UTILITY.SLASHCOMMAND_MENTION", success);
	}

#ifndef _WIN32
	set_test("TIMESTRINGTOTIMESTAMP", false);
	json tj;
	tj["t1"] = "2022-01-19T17:18:14.506000+00:00";
	tj["t2"] = "2022-01-19T17:18:14+00:00";
	uint32_t inTimestamp = 1642612694;
	set_test("TIMESTRINGTOTIMESTAMP", (uint64_t)dpp::ts_not_null(&tj, "t1") == inTimestamp && (uint64_t)dpp::ts_not_null(&tj, "t2") == inTimestamp);
#else
	set_test("TIMESTRINGTOTIMESTAMP", true);
#endif

	set_test("TS", false); 
	dpp::managed m(TEST_USER_ID);
	set_test("TS", ((uint64_t)m.get_creation_time()) == 1617131800);

	set_test("PRESENCE", false);
	set_test("CLUSTER", false);
	try {
		dpp::cluster bot(token, dpp::i_all_intents);
		bot.set_websocket_protocol(dpp::ws_etf);
		set_test("CLUSTER", true);
		set_test("CONNECTION", false);
		set_test("GUILDCREATE", false);
		set_test("ICONHASH", false);

		set_test("MSGCOLLECT", false);
		if (!offline) {
			/* Intentional leak: freed on unit test end */
			[[maybe_unused]]
			message_collector* collect_messages = new message_collector(&bot, 25);
		}

		dpp::utility::iconhash i;
		std::string dummyval("fcffffffffffff55acaaaaaaaaaaaa66");
		i = dummyval;
		set_test("ICONHASH", (i.to_string() == dummyval));

		/* This ensures we test both protocols, as voice is json and shard is etf */
		bot.set_websocket_protocol(dpp::ws_etf);

		bot.on_form_submit([&]([[maybe_unused]] const dpp::form_submit_t & event) {
		});

		/* This is near impossible to test without a 'clean room' voice channel.
		 * We attach this event just so that the decoder events are fired while we
		 * are sending audio later, this way if the audio receive code is plain unstable
		 * the test suite will crash and fail.
		 */
		bot.on_voice_receive_combined([&]([[maybe_unused]] auto& event) {
		});

		bot.on_ready([&bot]([[maybe_unused]] const dpp::ready_t & event) {

			set_test("CONNECTION", true);
			set_test("APPCOMMAND", false);
			set_test("LOGGER", false);

			bot.log(dpp::ll_info, "Test log message");

			bot.guild_command_create(dpp::slashcommand().set_name("testcommand")
				.set_description("Test command for DPP unit test")
				.add_option(dpp::command_option(dpp::co_attachment, "file", "a file"))
				.set_application_id(bot.me.id)
				.add_localization("fr", "zut", "Ou est la salor dans Discord?"),
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
										bot.message_delete(m.id, TEST_TEXT_CHANNEL_ID, [](const dpp::confirmation_callback_t &callback) {

											if (!callback.is_error()) {
												set_test("MESSAGEDELETE", true);
											} else {
												set_test("MESSAGEDELETE", false);
											}
										});
									} else {
										set_test("MESSAGECREATE", false);
									}
								});
							} else {
								set_test("DELCOMMAND", false);
							}
						});
					}
				});

			set_test("USER_GET", false);
			set_test("USER_GET_FLAGS", false);
			if (!offline) {
				bot.user_get(TEST_USER_ID, [](const dpp::confirmation_callback_t &event) {
					if (!event.is_error()) {
						auto u = std::get<dpp::user_identified>(event.value);
						if (u.id == TEST_USER_ID) {
							set_test("USER_GET", true);
						} else {
							set_test("USER_GET", false);
						}
						json j = json::parse(event.http_info.body);
						uint64_t raw_flags = j["public_flags"];
						if (j.contains("flags")) {
							uint64_t flags = j["flags"];
							raw_flags |= flags;
						}
						// testing all user flags from https://discord.com/developers/docs/resources/user#user-object-user-flags
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
							set_test("USER_GET_FLAGS", true);
						} else {
							set_test("USER_GET_FLAGS", false);
						}
					} else {
						set_test("USER_GET", false);
						set_test("USER_GET_FLAGS", false);
					}
				});
			}

			set_test("FORUM_CREATION", false);
			set_test("FORUM_CHANNEL_GET", false);
			set_test("FORUM_CHANNEL_DELETE", false);
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
						set_test("FORUM_CREATION", true);
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
								set_test("FORUM_CHANNEL_GET", tag && name && sort && rateLimit);
							} else {
								set_test("FORUM_CHANNEL_GET", false);
							}
							// delete the forum channel
							bot.channel_delete(forum_id, [](const dpp::confirmation_callback_t &event) {
								if (!event.is_error()) {
									set_test("FORUM_CHANNEL_DELETE", true);
								} else {
									set_test("FORUM_CHANNEL_DELETE", false);
								}
							});
						});
					} else {
						set_test("FORUM_CREATION", false);
						set_test("FORUM_CHANNEL_GET", false);
					}
				});
			}

			set_test("MEMBER_GET", false);
			if (!offline) {
				bot.guild_get_member(TEST_GUILD_ID, TEST_USER_ID, [](const dpp::confirmation_callback_t &event){
					if (!event.is_error()) {
						dpp::guild_member m = std::get<dpp::guild_member>(event.value);
						if (m.guild_id == TEST_GUILD_ID && m.user_id == TEST_USER_ID) {
							set_test("MEMBER_GET", true);
						} else {
							set_test("MEMBER_GET", false);
						}
					} else {
						set_test("MEMBER_GET", false);
					}
				});
			}

			set_test("ROLE_CREATE", false);
			set_test("ROLE_EDIT", false);
			set_test("ROLE_DELETE", false);
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
						set_test("ROLE_CREATE", true);
					}
				} catch (dpp::rest_exception &exception) {
					set_test("ROLE_CREATE", false);
				}
				createdRole.guild_id = TEST_GUILD_ID;
				createdRole.name = "Test-Role-Edited";
				createdRole.colour = dpp::colors::light_sea_green;
				try {
					dpp::role edited = bot.role_edit_sync(createdRole);
					if (createdRole.id == edited.id && edited.name == "Test-Role-Edited") {
						set_test("ROLE_EDIT", true);
					}
				} catch (dpp::rest_exception &exception) {
					set_test("ROLE_EDIT", false);
				}
				try {
					bot.role_delete_sync(TEST_GUILD_ID, createdRole.id);
					set_test("ROLE_DELETE", true);
				} catch (dpp::rest_exception &exception) {
					set_test("ROLE_DELETE", false);
				}
			}
		});

		std::mutex loglock;
		bot.on_log([&](const dpp::log_t & event) {
			std::lock_guard<std::mutex> locker(loglock);
			if (event.severity > dpp::ll_trace) {
				std::cout << "[" << std::fixed << std::setprecision(3) << (dpp::utility::time_f() - get_start_time()) << "]: [\u001b[36m" << dpp::utility::loglevel(event.severity) << "\u001b[0m] " << event.message << "\n";
			}
			if (event.message == "Test log message") {
				set_test("LOGGER", true);
			}
		});

		set_test("RUNONCE", false);
		uint8_t runs = 0;
		for (int x = 0; x < 10; ++x) {
			if (dpp::run_once<struct test_run>()) {
				runs++;
			}
		}
		set_test("RUNONCE", (runs == 1));

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

		set_test("SYNC", false);
		if (!offline) {
			dpp::message m = dpp::sync<dpp::message>(&bot, &dpp::cluster::message_create, dpp::message(TEST_TEXT_CHANNEL_ID, "TEST"));
			set_test("SYNC", m.content == "TEST");
		}

		bot.on_guild_create([&](const dpp::guild_create_t & event) {
			if (event.created->id == TEST_GUILD_ID) {
				set_test("GUILDCREATE", true);
				if (event.presences.size() && event.presences.begin()->second.user_id > 0) {
					set_test("PRESENCE", true);
				}
				dpp::guild* g = dpp::find_guild(TEST_GUILD_ID);
				set_test("CACHE", false);
				if (g) {
					set_test("CACHE", true);
					set_test("VOICECONN", false);
					dpp::discord_client* s = bot.get_shard(0);
					s->connect_voice(g->id, TEST_VC_ID, false, false);
				}
				else {
					set_test("CACHE", false);
				}
			}
		});

		bool message_tested = false;
		bot.on_message_create([&](const dpp::message_create_t & event) {
			if (event.msg.author.id == bot.me.id && !message_tested) {
				message_tested = true;
				set_test("MESSAGERECEIVE", true);
				set_test("MESSAGESGET", false);
				bot.messages_get(event.msg.channel_id, 0, event.msg.id, 0, 5, [](const dpp::confirmation_callback_t &cc){
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
				event.send("MSGCREATESEND", [&bot, ch_id = event.msg.channel_id] (const auto& cc) {
					if (!cc.is_error()) {
						dpp::message m = std::get<dpp::message>(cc.value);
						if (m.channel_id == ch_id) {
							set_test("MSGCREATESEND", true);
						} else {
							bot.log(dpp::ll_debug, cc.http_info.body);
							set_test("MSGCREATESEND", false);
						}
						bot.message_delete(m.id, m.channel_id);
					} else {
						bot.log(dpp::ll_debug, cc.http_info.body);
						set_test("MSGCREATESEND", false);
					}
				});
			}
		});

		set_test("BOTSTART", false);
		try {
			if (!offline) {
				bot.start(true);
				set_test("BOTSTART", true);
			}
		}
		catch (const std::exception &) {
			set_test("BOTSTART", false);
		}

		set_test("TIMERSTART", false);
		uint32_t ticks = 0;
		dpp::timer th = bot.start_timer([&]([[maybe_unused]] dpp::timer timer_handle) {
			if (ticks == 5) {
				/* The simple test timer ticks every second.
				 * If we get to 5 seconds, we know the timer is working.
				 */
				set_test("TIMERSTART", true);
			}
			ticks++;
		}, 1);

		set_test("TIMEDLISTENER", false);
		dpp::timed_listener tl(&bot, 10, bot.on_log, [&]([[maybe_unused]] const dpp::log_t & event) {
			set_test("TIMEDLISTENER", true);
		});

		set_test("ONESHOT", false);
		bool once = false;
		dpp::oneshot_timer ost(&bot, 5, [&]([[maybe_unused]] dpp::timer timer_handle) {
			if (!once) {
				set_test("ONESHOT", true);
			} else {
				set_test("ONESHOT", false);
			}
			once = true;
		});

		set_test("CUSTOMCACHE", false);
		dpp::cache<test_cached_object_t> testcache;
		test_cached_object_t* tco = new test_cached_object_t(666);
		tco->foo = "bar";
		testcache.store(tco);
		test_cached_object_t* found_tco = testcache.find(666);
		if (found_tco && found_tco->id == dpp::snowflake(666) && found_tco->foo == "bar") {
			set_test("CUSTOMCACHE", true);
		} else {
			set_test("CUSTOMCACHE", false);
		}
		testcache.remove(found_tco);

		noparam_api_test(current_user_get, dpp::user_identified, "CURRENTUSER");
		singleparam_api_test(channel_get, TEST_TEXT_CHANNEL_ID, dpp::channel, "GETCHAN");
		singleparam_api_test(guild_get, TEST_GUILD_ID, dpp::guild, "GETGUILD");
		singleparam_api_test_list(roles_get, TEST_GUILD_ID, dpp::role_map, "GETROLES");
		singleparam_api_test_list(channels_get, TEST_GUILD_ID, dpp::channel_map, "GETCHANS");
		singleparam_api_test_list(guild_get_invites, TEST_GUILD_ID, dpp::invite_map, "GETINVS");
		multiparam_api_test_list(guild_get_bans, TEST_GUILD_ID, dpp::ban_map, "GETBANS");
		singleparam_api_test_list(channel_pins_get, TEST_TEXT_CHANNEL_ID, dpp::message_map, "GETPINS");
		singleparam_api_test_list(guild_events_get, TEST_GUILD_ID, dpp::scheduled_event_map, "GETEVENTS");
		twoparam_api_test(guild_event_get, TEST_GUILD_ID, TEST_EVENT_ID, dpp::scheduled_event, "GETEVENT");
		twoparam_api_test_list(guild_event_users_get, TEST_GUILD_ID, TEST_EVENT_ID, dpp::event_member_map, "GETEVENTUSERS");

		std::this_thread::sleep_for(std::chrono::seconds(20));

		/* Test stopping timer */
		set_test("TIMERSTOP", false);
		set_test("TIMERSTOP", bot.stop_timer(th));

		set_test("USERCACHE", false);
		if (!offline) {
			dpp::user* u = dpp::find_user(TEST_USER_ID);
			set_test("USERCACHE", u);
		}
		set_test("CHANNELCACHE", false);
		set_test("CHANNELTYPES", false);
		if (!offline) {
			dpp::channel* c = dpp::find_channel(TEST_TEXT_CHANNEL_ID);
			dpp::channel* c2 = dpp::find_channel(TEST_VC_ID);
			set_test("CHANNELCACHE", c && c2);
			set_test("CHANNELTYPES", c && c->is_text_channel() && !c->is_voice_channel() && c2 && c2->is_voice_channel() && !c2->is_text_channel());
		}

		wait_for_tests();

	}
	catch (const std::exception &e) {
		std::cout << e.what() << "\n";
		set_test("CLUSTER", false);
	}

	/* Return value = number of failed tests, exit code 0 = success */
	return test_summary();
}
