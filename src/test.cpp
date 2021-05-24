#include <dpp/dpp.h>
#include <iostream>
#include <dpp/nlohmann/json.hpp>

using json = nlohmann::json;

int main()
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;

	dpp::cluster bot(configdocument["token"], dpp::i_default_intents | dpp::i_guild_members);
	dpp::commandhandler command_handler(&bot);

	command_handler.add_prefix(".");

	command_handler.add_command("ping", {
		{"testparameter", dpp::command_reg_param_t(dpp::pt_string, true, "Optional test parameter") }
	},
	[&command_handler](const std::string& command, const dpp::parameter_list_t& parameters, dpp::command_source src) {
		std::string got_param;
		if (!parameters.empty()) {
			got_param = std::get<std::string>(parameters[0].second);
		}
		command_handler.reply(dpp::message("Pong! -> " + got_param), src);
	});

	bot.on_log([](const dpp::log_t & event) {
		if (event.severity > dpp::ll_trace) {
			std::cout << event.message << "\n";
		}
	});

	bot.on_message_create([&bot, &command_handler](const dpp::message_create_t & event) {
		command_handler.route(*event.msg);
	});

	bot.start(false);
	return 0;
}
