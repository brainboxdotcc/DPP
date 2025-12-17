#include <dpp/dpp.h>
#include <iostream>

int main()
{
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* Use the on_user_context_menu event to look for user context menu actions */
	bot.on_user_context_menu([](const dpp::user_context_menu_t& event) {

		/* check if the context menu name is High Five */
		if (event.command.get_command_name() == "high five") {
			dpp::user user = event.get_user(); // the user who the command has been issued on
			dpp::user author = event.command.get_issuing_user(); // the user who clicked on the context menu
			event.reply(author.get_mention() + " slapped " + user.get_mention());
		}
	});

	bot.on_ready([&bot](const dpp::ready_t &event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create the command */
			dpp::slashcommand command;
			command.set_name("High Five")
				.set_application_id(bot.me.id)
				.set_type(dpp::ctxm_user);

			/* Register the command */
			bot.guild_command_create(command, 857692897221033129); /* Replace this with the guild id you want */
		}
	});

	/* Start bot */
	bot.start(dpp::st_wait);

	return 0;
}