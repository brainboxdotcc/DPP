\page commandhandler Using a command handler object

If you have many commands in your bot, and want to handle commands from multiple sources, you should consider instantiating a dpp::commandhandler object. This object can be used to automatically route
commands and their parameters to functions in your program. A simple example of using this object to route commands is shown below, and will
route both the /ping (global slash command) and .ping (prefixed channel message command) to a lambda where a reply can be generated.

\note This example automatically hooks the dpp::cluster::on_message_create and dpp::cluster::on_slashcommand events. This can be overridden if needed to allow you to still make use of these functions for your own code, if you need to do this please see the constructor documentation for dpp::commandhandler.

Note that because the dpp::commandhandler::add_command method accepts a `std::function` as the command handler, you may point a command handler
at a simple lambda (as shown in this example), a function pointer, or an instantiated class method of an object. This is extremely flexible
and allows you to decide how and where commands should be routed, either to an object oriented system or to a lambda based system.

\warning As of [August 30th, 2022](https://support-dev.discord.com/hc/en-us/articles/6025578854295-Why-We-Moved-to-Slash-Commands), you are advised to only be using slash commands, not messages for commands. To prevent the command handler from handling commands with messages, you should only use the "/" prefix. If you wish to still use messages for commands, this tutorial will still cover it but, again, it is discouraged by Discord.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main()
{
	/* If your bot only uses the "/" prefix, you can remove the intents here. */
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

	bot.on_log(dpp::utility::cout_logger());

	/* Create command handler, and specify prefixes */
	dpp::commandhandler command_handler(&bot);
	/* Specifying a prefix of "/" tells the command handler it should also expect slash commands. Remove the .add_prefix(".") if you wish to only make it a slash command */
	command_handler.add_prefix(".").add_prefix("/");

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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

