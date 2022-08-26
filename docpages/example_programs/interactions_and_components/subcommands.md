\page subcommands Using sub-commands in slash commands

This demonstrates how to use sub-commands within slash commands. Also shown below is an example of how to get a "resolved" parameter without having to use the cache or an extra API call.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>

int main() {

	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* Executes on ready. */
	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Define a slash command. */
			dpp::slashcommand image("image", "Send a specific image.", bot.me.id);
			image.add_option(
				/* Create a subcommand type option for "dog". */
				dpp::command_option(dpp::co_sub_command, "dog", "Send a picture of a dog.").
				add_option(dpp::command_option(dpp::co_user, "user", "User to turn into a dog.", false))
			);
			image.add_option(
				/* Create another subcommand type option for "cat". */
				dpp::command_option(dpp::co_sub_command, "cat", "Send a picture of a cat.").
				add_option(dpp::command_option(dpp::co_user, "user", "User to turn into a cat.", false))
			);
			/* Create command */
			bot.global_command_create(image);
		}
	});

	/* Use the on_slashcommand event to look for commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t & event) {
		dpp::interaction interaction = event.command;
		dpp::command_interaction cmd_data = interaction.get_command_interaction();
		/* Check if the command is the image command. */
		if (interaction.get_command_name() == "image") {
			/* Get the sub command */
			auto subcommand = cmd_data.options[0];
			/* Check if the subcommand is "dog" */
			if (subcommand.name == "dog") {	
				/* Checks if the subcommand has any options. */
				if (!subcommand.options.empty()) {
					/* Get the user from the parameter */
					dpp::user user = interaction.get_resolved_user(
						subcommand.get_option<dpp::snowflake>(0)
					);
					event.reply(user.get_mention() + " has now been turned into a dog."); 
				} else {
					/* Reply if there were no options.. */
					event.reply("No user specified");
				}
			}
			/* Check if the subcommand is "cat" */
			if (subcommand.name == "cat") {
				/* Checks if the subcommand has any options. */
				if (!subcommand.options.empty()) {
					/* Get the user from the parameter */
					dpp::user user = interaction.get_resolved_user(
						subcommand.get_option<dpp::snowflake>(0)
					);
					event.reply(user.get_mention() + " has now been turned into a cat."); 
				} else {
					/* Reply if there were no options.. */
					event.reply("No user specified");
				}
			}
		}
	});

	bot.start(dpp::st_wait);
	
	return 0;
} 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
