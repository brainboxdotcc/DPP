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

void utilities_unit_tests() {
	std::string text_to_escape = "*** _This is a test_ ***\n```cpp\n\
int main() {\n\
    /* Comment */\n\
    int answer = 42;\n\
    return answer; // ___\n\
};\n\
```\n\
Markdown lol ||spoiler|| ~~strikethrough~~ `small *code* block`\n";

	set_test(MD_ESC_1, false);
	set_test(MD_ESC_2, false);
	std::string escaped1 = dpp::utility::markdown_escape(text_to_escape);
	std::string escaped2 = dpp::utility::markdown_escape(text_to_escape, true);
	set_test(MD_ESC_1, escaped1 == "\\*\\*\\* \\_This is a test\\_ \\*\\*\\*\n\
```cpp\n\
int main() {\n\
    /* Comment */\n\
    int answer = 42;\n\
    return answer; // ___\n\
};\n\
```\n\
Markdown lol \\|\\|spoiler\\|\\| \\~\\~strikethrough\\~\\~ `small *code* block`\n");
	set_test(MD_ESC_2, escaped2 == "\\*\\*\\* \\_This is a test\\_ \\*\\*\\*\n\
\\`\\`\\`cpp\n\
int main\\(\\) {\n\
    /\\* Comment \\*/\n\
    int answer = 42;\n\
    return answer; // \\_\\_\\_\n\
};\n\
\\`\\`\\`\n\
Markdown lol \\|\\|spoiler\\|\\| \\~\\~strikethrough\\~\\~ \\`small \\*code\\* block\\`\n");

	set_test(URLENC, false);
	set_test(URLENC, dpp::utility::url_encode("ABC123_+\\|$*/AAA[]😄") == "ABC123_%2B%5C%7C%24%2A%2FAAA%5B%5D%F0%9F%98%84");

	set_test(BASE64ENC, false);
	set_test(BASE64ENC,
		 dpp::base64_encode(reinterpret_cast<unsigned char const*>("a"), 1) == "YQ==" &&
		 dpp::base64_encode(reinterpret_cast<unsigned char const*>("bc"), 2) == "YmM=" &&
		 dpp::base64_encode(reinterpret_cast<unsigned char const*>("def"), 3) == "ZGVm" &&
		 dpp::base64_encode(reinterpret_cast<unsigned char const*>("ghij"), 4) == "Z2hpag==" &&
		 dpp::base64_encode(reinterpret_cast<unsigned char const*>("klmno"), 5) == "a2xtbm8=" &&
		 dpp::base64_encode(reinterpret_cast<unsigned char const*>("pqrstu"), 6) == "cHFyc3R1" &&
		 dpp::base64_encode(reinterpret_cast<unsigned char const*>("vwxyz12"), 7) == "dnd4eXoxMg=="
	);

	set_test(TIMESTAMPTOSTRING, false);
	set_test(TIMESTAMPTOSTRING, dpp::ts_to_string(1642611864) == "2022-01-19T17:04:24Z");

	set_test(TIMESTRINGTOTIMESTAMP, false);
	json tj;
	tj["t1"] = "2022-01-19T17:18:14.506000+00:00";
	tj["t2"] = "2022-01-19T17:18:14+00:00";
	uint32_t inTimestamp = 1642612694;
	set_test(TIMESTRINGTOTIMESTAMP, (uint64_t)dpp::ts_not_null(&tj, "t1") == inTimestamp && (uint64_t)dpp::ts_not_null(&tj, "t2") == inTimestamp);

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

	set_test(COMPARISON, false);
	dpp::user u1;
	dpp::user u2;
	dpp::user u3;
	u1.id = u2.id = 666;
	u3.id = 777;
	set_test(COMPARISON, u1 == u2 && u1 != u3);
}
