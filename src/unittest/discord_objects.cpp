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

/* Unit tests for Discord objects (webhook, interaction, user etc.) */
void discord_objects_tests() {
	// test webhook
	set_test(WEBHOOK, false);
	try {
		dpp::webhook test_wh("https://discord.com/api/webhooks/833047646548133537/ntCHEYYIoHSLy_GOxPx6pmM0sUoLbP101ct-WI6F-S4beAV2vaIcl_Id5loAMyQwxqhE");
		set_test(WEBHOOK, (test_wh.token == "ntCHEYYIoHSLy_GOxPx6pmM0sUoLbP101ct-WI6F-S4beAV2vaIcl_Id5loAMyQwxqhE") && (test_wh.id == dpp::snowflake(833047646548133537)));
	}
	catch (const dpp::exception&) {
		set_test(WEBHOOK, false);
	}

	{ // test dpp::snowflake
		start_test(SNOWFLAKE);
		bool success = true;
		dpp::snowflake s = 69420;
		json j;
		j["value"] = s;
		success = dpp::snowflake_not_null(&j, "value") == 69420 && success;
		DPP_CHECK_CONSTRUCT_ASSIGN(SNOWFLAKE, dpp::snowflake, success);
		s = 42069;
		success = success && (s == 42069 && s == dpp::snowflake{42069} && s == "42069");
		success = success && (dpp::snowflake{69} < dpp::snowflake{420} && (dpp::snowflake{69} < 420));
		s = "69420";
		success = success && s == 69420;
		auto conversion_test = [](dpp::snowflake sl) {
		    return sl.str();
		};
		s = conversion_test(std::string{"1337"});
		success = success && s == 1337; /* THIS BREAKS (and i do not care very much): && s == conversion_test(dpp::snowflake{"1337"}); */
		success = success && dpp::snowflake{0} == 0;
		set_test(SNOWFLAKE, success);
	}

	{ // test interaction_create_t::get_parameter
		// create a fake interaction
		dpp::cluster cluster("");
		dpp::discord_client client(&cluster, 1, 1, "");
		dpp::interaction_create_t interaction(&client, "");

		/* Check the method with subcommands */
		set_test(GET_PARAMETER_WITH_SUBCOMMANDS, false);

		dpp::command_interaction cmd_data; // command
		cmd_data.type = dpp::ctxm_chat_input;
		cmd_data.name = "command";

		dpp::command_data_option subcommandgroup; // subcommand group
		subcommandgroup.name = "group";
		subcommandgroup.type = dpp::co_sub_command_group;

		dpp::command_data_option subcommand; // subcommand
		subcommand.name = "add";
		subcommand.type = dpp::co_sub_command;

		dpp::command_data_option option1; // slashcommand option
		option1.name = "user";
		option1.type = dpp::co_user;
		option1.value = dpp::snowflake(189759562910400512);

		dpp::command_data_option option2; // slashcommand option
		option2.name = "checked";
		option2.type = dpp::co_boolean;
		option2.value = true;

		// add them
		subcommand.options.push_back(option1);
		subcommand.options.push_back(option2);
		subcommandgroup.options.push_back(subcommand);
		cmd_data.options.push_back(subcommandgroup);
		interaction.command.data = cmd_data;

		dpp::snowflake value1 = std::get<dpp::snowflake>(interaction.get_parameter("user"));
		set_test(GET_PARAMETER_WITH_SUBCOMMANDS, value1 == dpp::snowflake(189759562910400512));

		/* Check the method without subcommands */
		set_test(GET_PARAMETER_WITHOUT_SUBCOMMANDS, false);

		dpp::command_interaction cmd_data2; // command
		cmd_data2.type = dpp::ctxm_chat_input;
		cmd_data2.name = "command";

		dpp::command_data_option option3; // slashcommand option
		option3.name = "number";
		option3.type = dpp::co_integer;
		option3.value = int64_t(123456);

		cmd_data2.options.push_back(option3);
		interaction.command.data = cmd_data2;

		int64_t value2 = std::get<int64_t>(interaction.get_parameter("number"));
		set_test(GET_PARAMETER_WITHOUT_SUBCOMMANDS, value2 == 123456);
	}

	{ // test dpp::command_option_choice::fill_from_json
		set_test(OPTCHOICE_DOUBLE, false);
		set_test(OPTCHOICE_INT, false);
		set_test(OPTCHOICE_BOOL, false);
		set_test(OPTCHOICE_SNOWFLAKE, false);
		set_test(OPTCHOICE_STRING, false);
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
		set_test(OPTCHOICE_DOUBLE, success_double);
		set_test(OPTCHOICE_INT, success_int && success_int2);
		set_test(OPTCHOICE_BOOL, success_bool);
		set_test(OPTCHOICE_SNOWFLAKE, success_snowflake);
		set_test(OPTCHOICE_STRING, success_string);
	}

	{ // test permissions
		set_test(PERMISSION_CLASS, false);
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
		p.set(0).add(~uint64_t{0}).remove(dpp::p_speak).set(dpp::p_administrator);
		success = !p.has(dpp::p_administrator, dpp::p_ban_members) && success; // must return false because they're not both set
		success = !p.has(dpp::p_administrator | dpp::p_ban_members) && success;
		success = p.can(dpp::p_ban_members) && success;
		success = p.can(dpp::p_speak) && success;

		constexpr auto permission_test = [](dpp::permission p) constexpr noexcept {
		    bool success{true};

		    p.set(0).add(~uint64_t{0}).remove(dpp::p_speak).set(dpp::p_connect);
		    p.set(dpp::p_administrator, dpp::p_ban_members);
		    success = p.has(dpp::p_administrator) && success;
		    success = p.has(dpp::p_administrator) && p.has(dpp::p_ban_members) && success;
		    success = p.has(dpp::p_administrator, dpp::p_ban_members) && success;
		    success = p.has(dpp::p_administrator | dpp::p_ban_members) && success;
		    success = p.add(dpp::p_speak).has(dpp::p_administrator, dpp::p_speak) && success;
		    success = !p.remove(dpp::p_speak).has(dpp::p_administrator, dpp::p_speak) && success;
		    p.remove(dpp::p_administrator);
		    success = p.can(dpp::p_ban_members) && success;
		    success = !p.can(dpp::p_speak, dpp::p_ban_members) && success;
		    success = p.can_any(dpp::p_speak, dpp::p_ban_members) && success;
		    return success;
		};
		constexpr auto constexpr_success = permission_test({~uint64_t{0}}); // test in constant evaluated
		success = permission_test({~uint64_t{0}}) && constexpr_success && success; // test at runtime
		set_test(PERMISSION_CLASS, success);
	}

	{ // some dpp::user methods
		dpp::user user1;
		user1.id = 189759562910400512;
		user1.discriminator = 0001;
		user1.username = "brain";

		set_test(USER_GET_MENTION, false);
		set_test(USER_GET_MENTION, user1.get_mention() == "<@189759562910400512>");

		set_test(USER_FORMAT_USERNAME, false);
		set_test(USER_FORMAT_USERNAME, user1.format_username() == "brain#0001");

		set_test(USER_GET_CREATION_TIME, false);
		set_test(USER_GET_CREATION_TIME, (uint64_t)user1.get_creation_time() == 1465312605);

		set_test(USER_GET_URL, false);

		dpp::user user2;
		set_test(USER_GET_URL,
			 user1.get_url() == dpp::utility::url_host + "/users/189759562910400512" &&
			 user2.get_url() == ""
		);
	}

	{ // avatar size function
		set_test(UTILITY_AVATAR_SIZE, false);
		bool success = false;
		success = dpp::utility::avatar_size(0).empty();
		success = dpp::utility::avatar_size(16) == "?size=16" && success;
		success = dpp::utility::avatar_size(256) == "?size=256" && success;
		success = dpp::utility::avatar_size(4096) == "?size=4096" && success;
		success = dpp::utility::avatar_size(8192).empty() && success;
		success = dpp::utility::avatar_size(3000).empty() && success;
		set_test(UTILITY_AVATAR_SIZE, success);
	}

	// some dpp::role test
	set_test(ROLE_COMPARE, false);
	dpp::role role_1, role_2;
	role_1.position = 1;
	role_2.position = 2;
	set_test(ROLE_COMPARE, role_1 < role_2 && role_1 != role_2);

	{ // message methods
		dpp::message m;
		m.guild_id = 825407338755653642;
		m.channel_id = 956230231277072415;
		m.id = 1151617986541666386;

		dpp::message m2;
		m2.guild_id = 825407338755653642;
		m2.channel_id = 956230231277072415;

		dpp::message m3;
		m3.guild_id = 825407338755653642;
		m3.id = 1151617986541666386;

		dpp::message m4;
		m4.channel_id = 956230231277072415;
		m4.id = 1151617986541666386;

		dpp::message m5;
		m5.guild_id = 825407338755653642;

		dpp::message m6;
		m6.channel_id = 956230231277072415;

		dpp::message m7;
		m7.id = 1151617986541666386;

		dpp::message m8;

		set_test(MESSAGE_GET_URL, false);
		set_test(MESSAGE_GET_URL,
			 m.get_url() == dpp::utility::url_host + "/channels/825407338755653642/956230231277072415/1151617986541666386" &&
			 m2.get_url() == "" &&
			 m3.get_url() == "" &&
			 m4.get_url() == "" &&
			 m5.get_url() == "" &&
			 m6.get_url() == "" &&
			 m7.get_url() == "" &&
			 m8.get_url() == ""
		);
	}

	{ // channel methods
		set_test(CHANNEL_SET_TYPE, false);
		dpp::channel c;
		c.set_flags(dpp::c_nsfw | dpp::c_video_quality_720p);
		c.set_type(dpp::CHANNEL_CATEGORY);
		bool before = c.is_category() && !c.is_forum();
		c.set_type(dpp::CHANNEL_FORUM);
		bool after = !c.is_category() && c.is_forum();
		set_test(CHANNEL_SET_TYPE, before && after);

		set_test(CHANNEL_GET_MENTION, false);
		c.id = 825411707521728511;
		set_test(CHANNEL_GET_MENTION, c.get_mention() == "<#825411707521728511>");

		set_test(CHANNEL_GET_URL, false);
		c.guild_id = 825407338755653642;

		dpp::channel c2;
		c2.id = 825411707521728511;

		dpp::channel c3;
		c3.guild_id = 825407338755653642;

		dpp::channel c4;

		set_test(CHANNEL_GET_URL,
			 c.get_url() == dpp::utility::url_host + "/channels/825407338755653642/825411707521728511" &&
			 c2.get_url() == "" &&
			 c3.get_url() == "" &&
			 c4.get_url() == ""
		);
	}

	{ // cdn endpoint url getter
		set_test(UTILITY_CDN_ENDPOINT_URL_HASH, false);
		bool success = false;
		success = dpp::utility::cdn_endpoint_url_hash({ dpp::i_png }, "foobar/test", "", dpp::i_jpg, 0).empty();
		success = dpp::utility::cdn_endpoint_url_hash({ dpp::i_png }, "foobar/test", "", dpp::i_png, 0) == "https://cdn.discordapp.com/foobar/test.png" && success;
		success = dpp::utility::cdn_endpoint_url_hash({ dpp::i_png }, "foobar/test", "", dpp::i_png, 128) == "https://cdn.discordapp.com/foobar/test.png?size=128" && success;
		success = dpp::utility::cdn_endpoint_url_hash({ dpp::i_png, dpp::i_gif }, "foobar/test", "12345", dpp::i_gif, 0, false, true) == "https://cdn.discordapp.com/foobar/test/a_12345.gif" && success;
		success = dpp::utility::cdn_endpoint_url_hash({ dpp::i_png, dpp::i_gif }, "foobar/test", "12345", dpp::i_png, 0, false, true) == "https://cdn.discordapp.com/foobar/test/a_12345.png" && success;
		success = dpp::utility::cdn_endpoint_url_hash({ dpp::i_png, dpp::i_gif }, "foobar/test", "12345", dpp::i_png, 0, false, false) == "https://cdn.discordapp.com/foobar/test/12345.png" && success;
		success = dpp::utility::cdn_endpoint_url_hash({ dpp::i_png, dpp::i_gif }, "foobar/test", "12345", dpp::i_png, 0, true, true) == "https://cdn.discordapp.com/foobar/test/a_12345.gif" && success;
		success = dpp::utility::cdn_endpoint_url_hash({ dpp::i_png, dpp::i_gif }, "foobar/test", "", dpp::i_png, 0, true, true) == "https://cdn.discordapp.com/foobar/test.gif" && success;
		success = dpp::utility::cdn_endpoint_url_hash({ dpp::i_png, dpp::i_gif }, "foobar/test", "", dpp::i_gif, 0, false, false).empty() && success;
		set_test(UTILITY_CDN_ENDPOINT_URL_HASH, success);
	}

	{ // user url getter
		dpp::user user1;
		user1.id = 189759562910400512;
		user1.username = "Brain";
		user1.discriminator = 0001;

		auto user2 = user1;
		user2.avatar = "5532c6414c70765a28cf9448c117205f";

		auto user3 = user2;
		user3.flags |= dpp::u_animated_icon;

		set_test(USER_GET_AVATAR_URL, false);
		set_test(USER_GET_AVATAR_URL,
			 dpp::user().get_avatar_url().empty() &&
			 user1.get_avatar_url() == dpp::utility::cdn_host + "/embed/avatars/1.png" &&
			 user2.get_avatar_url() == dpp::utility::cdn_host + "/avatars/189759562910400512/5532c6414c70765a28cf9448c117205f.png" &&
			 user2.get_avatar_url(0, dpp::i_webp) == dpp::utility::cdn_host + "/avatars/189759562910400512/5532c6414c70765a28cf9448c117205f.webp" &&
			 user2.get_avatar_url(0, dpp::i_jpg) == dpp::utility::cdn_host + "/avatars/189759562910400512/5532c6414c70765a28cf9448c117205f.jpg" &&
			 user3.get_avatar_url() == dpp::utility::cdn_host + "/avatars/189759562910400512/a_5532c6414c70765a28cf9448c117205f.gif" &&
			 user3.get_avatar_url(4096, dpp::i_gif) == dpp::utility::cdn_host + "/avatars/189759562910400512/a_5532c6414c70765a28cf9448c117205f.gif?size=4096" &&
			 user3.get_avatar_url(512, dpp::i_webp) == dpp::utility::cdn_host + "/avatars/189759562910400512/a_5532c6414c70765a28cf9448c117205f.gif?size=512" &&
			 user3.get_avatar_url(512, dpp::i_jpg) == dpp::utility::cdn_host + "/avatars/189759562910400512/a_5532c6414c70765a28cf9448c117205f.gif?size=512" &&
			 user3.get_avatar_url(16, dpp::i_jpg, false) == dpp::utility::cdn_host + "/avatars/189759562910400512/a_5532c6414c70765a28cf9448c117205f.jpg?size=16" &&
			 user3.get_avatar_url(5000) == dpp::utility::cdn_host + "/avatars/189759562910400512/a_5532c6414c70765a28cf9448c117205f.gif"
		);
	}

	{ // sticker url getter
		set_test(STICKER_GET_URL, false);
		dpp::sticker s;
		s.format_type = dpp::sf_png;
		bool success = s.get_url().empty();
		s.id = 12345;
		success = s.get_url() == "https://cdn.discordapp.com/stickers/12345.png" && success;
		s.format_type = dpp::sf_gif;
		success = s.get_url() == "https://cdn.discordapp.com/stickers/12345.gif" && success;
		s.format_type = dpp::sf_lottie;
		success = s.get_url() == "https://cdn.discordapp.com/stickers/12345.json" && success;
		set_test(STICKER_GET_URL, success);
	}

	{ // emoji url getter
		dpp::emoji emoji;
		emoji.id = 825407338755653641;

		set_test(EMOJI_GET_URL, false);
		set_test(EMOJI_GET_URL, emoji.get_url() == dpp::utility::cdn_host + "/emojis/825407338755653641.png");
	}

	{ // utility methods
		set_test(UTILITY_GUILD_NAVIGATION, false);
		auto gn1 = dpp::utility::guild_navigation(123, dpp::utility::gnt_customize);
		auto gn2 = dpp::utility::guild_navigation(1234, dpp::utility::gnt_browse);
		auto gn3 = dpp::utility::guild_navigation(12345, dpp::utility::gnt_guide);
		set_test(UTILITY_GUILD_NAVIGATION, gn1 == "<123:customize>" && gn2 == "<1234:browse>" && gn3 == "<12345:guide>");

		set_test(UTILITY_ICONHASH, false);
		auto iconhash1 = dpp::utility::iconhash("a_5532c6414c70765a28cf9448c117205f");
		set_test(UTILITY_ICONHASH, iconhash1.first == 6139187225817019994 &&
					   iconhash1.second == 2940732121894297695 &&
					   iconhash1.to_string() == "5532c6414c70765a28cf9448c117205f"
		);

		set_test(UTILITY_MAKE_URL_PARAMETERS, false);
		auto url_params1 = dpp::utility::make_url_parameters({
									     {"foo", 15},
									     {"bar", 7}
								     });
		auto url_params2 = dpp::utility::make_url_parameters({
									     {"foo", "hello"},
									     {"bar", "two words"}
								     });
		set_test(UTILITY_MAKE_URL_PARAMETERS, url_params1 == "?bar=7&foo=15" && url_params2 == "?bar=two%20words&foo=hello");

		set_test(UTILITY_MARKDOWN_ESCAPE, false);
		auto escaped = dpp::utility::markdown_escape(
			"> this is a quote\n"
			"**some bold text**");
		set_test(UTILITY_MARKDOWN_ESCAPE, "\\>this is a quote\\n\\*\\*some bold text\\*\\*");

		set_test(UTILITY_TOKENIZE, false);
		auto tokens = dpp::utility::tokenize("some Whitespace seperated Text to Tokenize", " ");
		std::vector<std::string> expected_tokens = {"some", "Whitespace", "seperated", "Text", "to", "Tokenize"};
		set_test(UTILITY_TOKENIZE, tokens == expected_tokens);

		set_test(UTILITY_URL_ENCODE, false);
		auto url_encoded = dpp::utility::url_encode("S2-^$1Nd+U!g'8+_??o?p-bla bla");
		set_test(UTILITY_URL_ENCODE, url_encoded == "S2-%5E%241Nd%2BU%21g%278%2B_%3F%3Fo%3Fp-bla%20bla");

		set_test(UTILITY_SLASHCOMMAND_MENTION, false);
		auto mention1 = dpp::utility::slashcommand_mention(123, "name");
		auto mention2 = dpp::utility::slashcommand_mention(123, "name", "sub");
		auto mention3 = dpp::utility::slashcommand_mention(123, "name", "group", "sub");
		bool success = mention1 == "</name:123>" && mention2 == "</name sub:123>" && mention3 == "</name group sub:123>";
		set_test(UTILITY_SLASHCOMMAND_MENTION, success);

		set_test(UTILITY_CHANNEL_MENTION, false);
		auto channel_mention = dpp::utility::channel_mention(123);
		set_test(UTILITY_CHANNEL_MENTION, channel_mention == "<#123>");

		set_test(UTILITY_USER_MENTION, false);
		auto user_mention = dpp::utility::user_mention(123);
		set_test(UTILITY_USER_MENTION, user_mention == "<@123>");

		set_test(UTILITY_ROLE_MENTION, false);
		auto role_mention = dpp::utility::role_mention(123);
		set_test(UTILITY_ROLE_MENTION, role_mention == "<@&123>");

		set_test(UTILITY_EMOJI_MENTION, false);
		auto emoji_mention1 = dpp::utility::emoji_mention("role1", 123, false);
		auto emoji_mention2 = dpp::utility::emoji_mention("role2", 234, true);
		auto emoji_mention3 = dpp::utility::emoji_mention("white_check_mark", 0, false);
		auto emoji_mention4 = dpp::utility::emoji_mention("white_check_mark", 0, true);
		set_test(UTILITY_EMOJI_MENTION,
			 emoji_mention1 == "<:role1:123>" &&
			 emoji_mention2 == "<a:role2:234>" &&
			 emoji_mention3 == ":white_check_mark:" &&
			 emoji_mention4 == ":white_check_mark:"
		);

		set_test(UTILITY_USER_URL, false);
		auto user_url = dpp::utility::user_url(123);
		set_test(UTILITY_USER_URL,
			 user_url == dpp::utility::url_host + "/users/123" &&
			 dpp::utility::user_url(0) == ""
		);

		set_test(UTILITY_MESSAGE_URL, false);
		auto message_url = dpp::utility::message_url(1,2,3);
		set_test(UTILITY_MESSAGE_URL,
			 message_url == dpp::utility::url_host+ "/channels/1/2/3" &&
			 dpp::utility::message_url(0,2,3) == "" &&
			 dpp::utility::message_url(1,0,3) == "" &&
			 dpp::utility::message_url(1,2,0) == "" &&
			 dpp::utility::message_url(0,0,3) == "" &&
			 dpp::utility::message_url(0,2,0) == "" &&
			 dpp::utility::message_url(1,0,0) == "" &&
			 dpp::utility::message_url(0,0,0) == ""
		);

		set_test(UTILITY_CHANNEL_URL, false);
		auto channel_url = dpp::utility::channel_url(1,2);
		set_test(UTILITY_CHANNEL_URL,
			 channel_url == dpp::utility::url_host+ "/channels/1/2" &&
			 dpp::utility::channel_url(0,2) == "" &&
			 dpp::utility::channel_url(1,0) == "" &&
			 dpp::utility::channel_url(0,0) == ""
		);

		set_test(UTILITY_THREAD_URL, false);
		auto thread_url = dpp::utility::thread_url(1,2);
		set_test(UTILITY_THREAD_URL,
			 thread_url == dpp::utility::url_host+ "/channels/1/2" &&
			 dpp::utility::thread_url(0,2) == "" &&
			 dpp::utility::thread_url(1,0) == "" &&
			 dpp::utility::thread_url(0,0) == ""
		);
	}

	{ // dpp event classes
		start_test(EVENT_CLASS);
		bool success = true;
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::log_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_scheduled_event_user_add_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_scheduled_event_user_remove_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_scheduled_event_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_scheduled_event_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_scheduled_event_delete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::automod_rule_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::automod_rule_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::automod_rule_delete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::automod_rule_execute_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::stage_instance_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::stage_instance_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::stage_instance_delete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::voice_state_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::interaction_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::slashcommand_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::button_click_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::form_submit_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::autocomplete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::context_menu_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::message_context_menu_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::user_context_menu_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::select_click_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_delete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_stickers_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_join_request_delete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::channel_delete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::channel_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::ready_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::message_delete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_member_remove_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::resumed_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_role_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::typing_start_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::voice_track_marker_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::message_reaction_add_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_members_chunk_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::message_reaction_remove_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::channel_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::message_reaction_remove_emoji_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::message_delete_bulk_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_role_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_role_delete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::channel_pins_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::message_reaction_remove_all_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::voice_server_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_emojis_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::presence_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::webhooks_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_member_add_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::invite_delete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_integrations_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_member_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::invite_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::message_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::user_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::message_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_audit_log_entry_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_ban_add_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::guild_ban_remove_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::integration_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::integration_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::integration_delete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::thread_create_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::thread_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::thread_delete_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::thread_list_sync_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::thread_member_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::thread_members_update_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::voice_buffer_send_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::voice_user_talking_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::voice_ready_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::voice_receive_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::voice_client_speaking_t, success);
		DPP_CHECK_CONSTRUCT_ASSIGN(EVENT_CLASS, dpp::voice_client_disconnect_t, success);
		set_test(EVENT_CLASS, success);
	}
}
