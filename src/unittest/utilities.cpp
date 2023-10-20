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

/**
 * @brief Type trait to check if a certain type has a build_json method
 *
 * @tparam T type to check for
 */
template <typename T, typename = std::void_t<>>
struct has_build_json : std::false_type {};

template <typename T>
struct has_build_json<T, std::void_t<decltype(std::declval<T&>().build_json())>> : std::true_type {};

/**
 * @brief Type trait to check if a certain type has a build_json method
 *
 * @tparam T type to check for
 */
template <typename T>
constexpr bool has_build_json_v = has_build_json<T>::value;

/**
 * @brief Type trait to check if a certain type has a fill_from_json method
 *
 * @tparam T type to check for
 */
template <typename T, typename = void>
struct has_fill_from_json : std::false_type {};

template <typename T>
struct has_fill_from_json<T, std::void_t<decltype(std::declval<T&>().fill_from_json(std::declval<dpp::json*>()))>> : std::true_type {};

/**
 * @brief Type trait to check if a certain type has a fill_from_json method
 *
 * @tparam T type to check for
 */
template <typename T>
constexpr bool has_fill_from_json_v = has_fill_from_json<T>::value;

/* Unit tests for library utilities */
void utility_tests() {
	// markdown escape tests
	std::string test_to_escape = "*** _This is a test_ ***\n```cpp\n\
int main() {\n\
    /* Comment */\n\
    int answer = 42;\n\
    return answer; // ___\n\
};\n\
```\n\
Markdown lol ||spoiler|| ~~strikethrough~~ `small *code* block`\n";

	set_test(MD_ESC_1, false);
	set_test(MD_ESC_2, false);
	std::string escaped1 = dpp::utility::markdown_escape(test_to_escape);
	std::string escaped2 = dpp::utility::markdown_escape(test_to_escape, true);
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


	set_test(COMPARISON, false);
	dpp::user u1;
	dpp::user u2;
	dpp::user u3;
	u1.id = u2.id = 666;
	u3.id = 777;
	set_test(COMPARISON, u1 == u2 && u1 != u3);

	// encoding tests
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

	set_test(READFILE, false);
	std::string rf_test = dpp::utility::read_file(SHARED_OBJECT);
	FILE* fp = fopen(SHARED_OBJECT, "rb");
	fseek(fp, 0, SEEK_END);
	size_t off = (size_t)ftell(fp);
	fclose(fp);
	set_test(READFILE, off == rf_test.length());

	set_test(TIMESTAMPTOSTRING, false);
	set_test(TIMESTAMPTOSTRING, dpp::ts_to_string(1642611864) == "2022-01-19T17:04:24Z");

#ifndef _WIN32
	set_test(TIMESTRINGTOTIMESTAMP, false);
	json tj;
	tj["t1"] = "2022-01-19T17:18:14.506000+00:00";
	tj["t2"] = "2022-01-19T17:18:14+00:00";
	uint32_t inTimestamp = 1642612694;
	set_test(TIMESTRINGTOTIMESTAMP, (uint64_t)dpp::ts_not_null(&tj, "t1") == inTimestamp && (uint64_t)dpp::ts_not_null(&tj, "t2") == inTimestamp);
#else
	set_test(TIMESTRINGTOTIMESTAMP, true);
#endif

	{
		set_test(TS, false);
		dpp::managed m(189759562910400512);
		set_test(TS, ((uint64_t) m.get_creation_time()) == 1465312605);
	}

	{ // test dpp::json_interface
		start_test(JSON_INTERFACE);
		struct fillable : dpp::json_interface<fillable> {
			fillable &fill_from_json_impl(dpp::json *) {
				return *this;
			}
		};
		struct buildable : dpp::json_interface<buildable> {
			json to_json_impl(bool = false) const {
				return {};
			}
		};
		struct fillable_and_buildable : dpp::json_interface<fillable_and_buildable> {
			fillable_and_buildable &fill_from_json_impl(dpp::json *) {
				return *this;
			}

			json to_json_impl(bool = false) const {
				return {};
			}
		};
		bool success = true;

		DPP_CHECK(JSON_INTERFACE, has_build_json_v<dpp::json_interface<buildable>>, success);
		DPP_CHECK(JSON_INTERFACE, !has_fill_from_json_v<dpp::json_interface<buildable>>, success);
		DPP_CHECK(JSON_INTERFACE, has_build_json_v<buildable>, success);
		DPP_CHECK(JSON_INTERFACE, !has_fill_from_json_v<buildable>, success);

		DPP_CHECK(JSON_INTERFACE, !has_build_json_v<dpp::json_interface<fillable>>, success);
		DPP_CHECK(JSON_INTERFACE, has_fill_from_json_v<dpp::json_interface<fillable>>, success);
		DPP_CHECK(JSON_INTERFACE, !has_build_json_v<fillable>, success);
		DPP_CHECK(JSON_INTERFACE, has_fill_from_json_v<fillable>, success);

		DPP_CHECK(JSON_INTERFACE, has_build_json_v<dpp::json_interface<fillable_and_buildable>>, success);
		DPP_CHECK(JSON_INTERFACE, has_fill_from_json_v<dpp::json_interface<fillable_and_buildable>>, success);
		DPP_CHECK(JSON_INTERFACE, has_build_json_v<fillable_and_buildable>, success);
		DPP_CHECK(JSON_INTERFACE, has_fill_from_json_v<fillable_and_buildable>, success);
		set_test(JSON_INTERFACE, success);
	}
}
