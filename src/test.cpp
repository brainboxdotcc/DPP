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
#include <dpp/dpp.h>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

int main(int argc, char const *argv[])
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;

	dpp::cluster bot(configdocument["token"], dpp::i_default_intents | dpp::i_guild_members, 1);

	bot.on_log([&bot](const dpp::log_t & event) {
		if (event.severity >= dpp::ll_debug) {
			std::cout << dpp::utility::current_date_time() << " [" << dpp::utility::loglevel(event.severity) << "] " << event.message << "\n";
		}
	});

	bot.on_interaction_create([&bot](const dpp::interaction_create_t & cmd) {
		if (cmd.command.data.name == "blep") {
			std::string animal = std::get<std::string>(cmd.get_parameter("animal"));
			cmd.reply(dpp::ir_channel_message_with_source, fmt::format("Blep! You chose {}", animal));
		}
	});

	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		std::stringstream ss(event.msg->content);
		std::string command;
		ss >> command;

		dpp::snowflake channel_id = event.msg->channel_id;

		if (command == ".cmtest") {
			std::map<dpp::snowflake, dpp::guild_member*> ml = dpp::find_channel(825841181342171196)->get_members();
			for (auto & m : ml) {
				std::cout << m.second->user_id << "\n";
			}
		}

		if (command == ".exectest") {
			dpp::utility::exec("ping", {"-c", "4", "127.0.0.1"}, [&bot, channel_id](const std::string& output) {
				bot.message_create(dpp::message(channel_id, output));
			});
		}

		if (command == ".createslash") {
			dpp::slashcommand newcommand;
			newcommand.set_name("blep").set_description("Send a random adorable animal photo").set_application_id(bot.me.id);
			newcommand.add_option(
				dpp::command_option(dpp::co_string, "animal", "The type of animal", true).
					add_choice(dpp::command_option_choice("Dog", std::string("animal_dog"))).
					add_choice(dpp::command_option_choice("Cat", std::string("animal_cat"))).
					add_choice(dpp::command_option_choice("Penguin", std::string("animal_penguin"))
				)
			).add_option(
				dpp::command_option(dpp::co_boolean, "only_smol", "Whether to show only baby animals")
			);
			std::cout << newcommand.build_json(false) << "\n";
			bot.guild_command_create(newcommand, event.msg->guild_id, [&bot](const dpp::confirmation_callback_t & state) {
				bot.log(dpp::ll_debug, fmt::format("Application command tried. Result: {} -> {}", state.http_info.status, state.http_info.body));
			});
		}
	});

	/* This method call actually starts the bot by connecting all shards in the cluster */
	bot.start(false);
	
	/* Never reached */
	return 0;
}

