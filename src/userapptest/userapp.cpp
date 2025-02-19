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

int main() {
	char* t = getenv("DPP_UNIT_TEST_TOKEN");

	if (!t) {
		std::cerr << "Missing DPP_UNIT_TEST_TOKEN\n";
		exit(1);
	}

	dpp::cluster bot(t, dpp::i_default_intents);
	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const auto& event) {
		if (dpp::run_once<struct boot_t>()) {
			/**
			 * Create a slash command which has interaction context 'itc_private_channel'.
			 * This is a user-app command which can be executed anywhere and is added to the user's profile.
			 */
			bot.global_bulk_command_create({
				dpp::slashcommand("userapp", "Test command", bot.me.id)
				.set_interaction_contexts({dpp::itc_guild, dpp::itc_bot_dm, dpp::itc_private_channel})
			});
		}
	});

	bot.register_command("userapp", [](const dpp::slashcommand_t& e) {
		e.reply(dpp::message().set_flags(dpp::m_using_components_v2).add_component(
			dpp::component()
				.set_type(dpp::cot_container)
				.set_accent(dpp::utility::rgb(255, 0, 0))
				.set_spoiler(true)
				.add_component_v2(
					dpp::component().set_type(dpp::cot_section).add_component_v2(
						dpp::component()
							.set_type(dpp::cot_text_display)
							.set_content("This is text to display")
					)
					.set_accessory(
						dpp::component()
							.set_type(dpp::cot_button)
							.set_label("Click me")
							.set_style(dpp::cos_danger)
							.set_id("button")
					)
				)
		));
	});

	bot.start(dpp::st_wait);
}
