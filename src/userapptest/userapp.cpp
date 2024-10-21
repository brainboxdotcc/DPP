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
				dpp::slashcommand("userapp", "Test user app command", bot.me.id)
				.set_interaction_contexts({dpp::itc_guild, dpp::itc_bot_dm, dpp::itc_private_channel})
			});
		}
	});

	bot.register_command("userapp", [](const dpp::slashcommand_t& e) {
		/**
		 * Simple test output that shows the context of the command
		 */
		e.reply("This is the `/userapp` command." + std::string(
			e.command.is_user_app_interaction() ?
			" Executing as a user interaction owned by user: <@" + e.command.get_authorizing_integration_owner(dpp::ait_user_install).str() + ">" :
			" Executing as a guild interaction on guild id " + e.command.guild_id.str()
		));
	});

	bot.start(dpp::st_wait);
}
