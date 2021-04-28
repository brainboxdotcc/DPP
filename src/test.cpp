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

	dpp::cluster bot(configdocument["token"], dpp::i_default_intents | dpp::i_guild_members, 48, 0, 12);

	bot.on_log([&bot](const dpp::log_t & event) {
		if (event.severity >= dpp::ll_debug) {
			std::cout << dpp::utility::current_date_time() << " [" << dpp::utility::loglevel(event.severity) << "] " << event.message << "\n";
		}
	});

	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		std::stringstream ss(event.msg->content);
		std::string command;
		ss >> command;
	});

	/* This method call actually starts the bot by connecting all shards in the cluster */
	bot.start(false);
	
	/* Never reached */
	return 0;
}

