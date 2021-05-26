#include <dpp/dpp.h>
#include <iostream>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt/format.h>

using json = nlohmann::json;

int main()
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;
	dpp::cluster bot(configdocument["token"], dpp::i_default_intents | dpp::i_guild_members);

	/* Create command handler, and specify prefixes */
	dpp::commandhandler command_handler(&bot);
	/* Specifying a prefix of "/" tells the command handler it should also expect slash commands */
	command_handler.add_prefix(".").add_prefix("/");

	bot.on_ready([&command_handler](const dpp::ready_t &event) {

		dpp::discord_client* d = event.from;

		command_handler.add_command(
			/* Command name */
			"ping",

			/* Parameters */
			{
				{"testparameter", dpp::param_info(dpp::pt_string, true, "Optional test parameter") }
			},

			/* Command handler */
			[&command_handler, d](const std::string& command, const dpp::parameter_list_t& parameters, dpp::command_source src) {

				command_handler.reply(dpp::message(fmt::format("Pong! {0:.3f}", d->websocket_ping)), src);
			},

			/* Command description */
			"A test ping command",

			/* Guild id (omit for a global command) */
			819556414099554344
		);

	});


	bot.on_log([](const dpp::log_t & event) {
		if (event.severity > dpp::ll_trace) {
			std::cout << event.message << "\n";
		}
	});

	bot.start(false);
	return 0;
}
