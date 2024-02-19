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
}
