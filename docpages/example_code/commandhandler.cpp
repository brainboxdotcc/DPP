#include <dpp/dpp.h>

int main() {
	/* If your bot only uses the "/" prefix, you can remove the intents here. */
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

	bot.on_log(dpp::utility::cout_logger()); 

	/* Create command handler, and specify prefixes */
	dpp::commandhandler command_handler(&bot);
	/* Specifying a prefix of "/" tells the command handler it should also expect slash commands. Remove the .add_prefix(".") if you wish to only make it a slash command */
	command_handler.add_prefix(".")
		.add_prefix("/");

	bot.on_ready([&command_handler](const dpp::ready_t &event) {

		command_handler.add_command(
			/* Command name */
			"ping",

			/* Parameters */
			{
				{"testparameter", dpp::param_info(dpp::pt_string, true, "Optional test parameter") }
			},

			/* Command handler */
			[&command_handler](const std::string& command, const dpp::parameter_list_t& parameters, dpp::command_source src) {
				std::string got_param;
				if (!parameters.empty()) {
					got_param = std::get<std::string>(parameters[0].second);
				}
				command_handler.reply(dpp::message("Pong! -> " + got_param), src);
			},

			/* Command description */
			"A test ping command",

			/* Guild id (omit for a guild command) */
			819556414099554344
		);

		/* NOTE: We must call this to ensure slash commands are registered.
		 * This does a bulk register, which will replace other commands
		 * that are registered already!
		 */
		command_handler.register_commands();

	});

	bot.start(dpp::st_wait);

	return 0;
}
