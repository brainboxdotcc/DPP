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
}
