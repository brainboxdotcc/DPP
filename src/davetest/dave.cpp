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
#include <dpp/dpp.h>
#include <iostream>
#include <string>

std::string get_testdata_dir() {
	char *env_var = getenv("TEST_DATA_DIR");
	return (env_var ? env_var : "../../testdata/");
}

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

int main() {
	
	using namespace std::chrono_literals;
	char* t = getenv("DPP_UNIT_TEST_TOKEN");
	if (t == nullptr || getenv("TEST_GUILD_ID") == nullptr || getenv("TEST_VC_ID") == nullptr) {
		std::cerr << "Missing unit test environment. Set DPP_UNIT_TEST_TOKEN, TEST_GUILD_ID, and TEST_VC_ID\n";
		exit(1);
	}
	dpp::snowflake TEST_GUILD_ID(std::string(getenv("TEST_GUILD_ID")));
	dpp::snowflake TEST_VC_ID(std::string(getenv("TEST_VC_ID")));
	std::cout << "Test Guild ID: " << TEST_GUILD_ID << " Test VC ID: " << TEST_VC_ID << "\n\n";
	dpp::cluster dave_test(t, dpp::i_default_intents, 1, 0, 1, false, dpp::cache_policy_t{ dpp::cp_none, dpp::cp_none, dpp::cp_none, dpp::cp_none, dpp::cp_none });

	dave_test.on_log([&](const dpp::log_t& log) {
		std::cout << "[" << dpp::utility::current_date_time() << "] " << dpp::utility::loglevel(log.severity) << ": " << log.message << std::endl;
	});

	std::vector<uint8_t> testaudio = load_test_audio();

	dave_test.on_voice_ready([&](const dpp::voice_ready_t & event) {
		dave_test.log(dpp::ll_info, "Voice channel ready, sending audio...");
		dpp::discord_voice_client* v = event.voice_client;
		if (v && v->is_ready()) {
			v->send_audio_raw((uint16_t*)testaudio.data(), testaudio.size());
		}
		dave_test.start_timer([v, &testaudio](auto) {
			v->send_audio_raw((uint16_t*)testaudio.data(), testaudio.size());
		}, 15);
	});


	dave_test.on_guild_create([&](const dpp::guild_create_t & event) {
		if (event.created.id == TEST_GUILD_ID) {
			dpp::discord_client* s = event.from();
			bool muted = false, deaf = false, enable_dave = true;
			s->connect_voice(TEST_GUILD_ID, TEST_VC_ID, muted, deaf, enable_dave);
		}
	});
	dave_test.start(dpp::st_wait);
}
