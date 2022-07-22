\page subcommands Using sub-commands in slash commands

This is how to use Subcommands within your Slash Commands for your bots.

To make a subcomamnd within your command use this
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <dpp/fmt/format.h>
#include <iostream>

int main() {

	/* Setup the bot */
	dpp::cluster bot("token");
	
        bot.on_log(dpp::utility::cout_logger());

	/* Executes on ready. */
	bot.on_ready([&bot](const dpp::ready_t & event) {
	    if (dpp::run_once<struct register_bot_commands>()) {
	        // Define a slash command.
	        dpp::slashcommand image("image", "Send a specific image.", bot.me.id);
	        image.add_option(
	    		// Create a subcommand type option.
 	           	dpp::command_option(dpp::co_sub_command, "dog", "Send a picture of a dog.").
	    			add_option(dpp::command_option(dpp::co_user, "user", "User to make a dog off.", false))
	    		);
	    	image.add_option(
	    		// Create another subcommand type option.
	            dpp::command_option(dpp::co_sub_command, "cat", "Send a picture of a cat.").
	    			add_option(dpp::command_option(dpp::co_user, "user", "User to make a cat off.", false))
	    		);
	        // Create command with a callback.
	        bot.global_command_create(image, [ & ]( const dpp::confirmation_callback_t &callback ) {
	            if (callback.is_error()) {
	                std::cout << callback.http_info.body <<  "\n" ;
	            }
	    	});
	    }
	});

	/* Use the on_slashcommand event to look for commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t & event) {
		dpp::command_interaction cmd_data = event.command.get_command_interaction();
		/* Check if the command is the image command. */
		if (event.command.get_command_name() == "image") {
			/* Check if the subcommand is "dog" */
			if (cmd_data.options[0].name == "dog") {	
				/* Checks if the subcommand has any options. */
				if (cmd_data.options[0].options.size() > 0) {
					/* Get the user option as a snowflake. */
					dpp::snowflake user = std::get<dpp::snowflake>(cmd_data.options[0].options[0].value);
					event.reply(fmt::format("<@{}> has now been turned into a dog.", user)); 
				} else {
					/* Reply if there were no options.. */
					event.reply("<A picture of a dog.>");
				}
			}
			/* Check if the subcommand is "cat" */
			if (cmd_data.options[0].name == "cat") {
				/* Checks if the subcommand has any options. */
				if (cmd_data.options[0].options.size() > 0) {
					/* Get the user option as a snowflake. */
					dpp::snowflake user = std::get<dpp::snowflake>(cmd_data.options[0].options[0].value);
					event.reply(fmt::format("<@{}> has now been turned into a cat.", user));
				} else {
					/* Reply if there were no options.. */
					event.reply("<A picture of a cat.>");
				}
			}
		}
	});

	bot.start(false);
	
	return 0;
} 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
