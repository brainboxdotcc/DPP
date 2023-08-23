\page discord-application-command-file-upload Using file parameters for application commands (slash commands)

The program below demonstrates how to use the 'file' type parameter to an application command (slash command).
You must first get the file_id via `std::get`, and then you can find the attachment details in the 'resolved'
section, `event.command.resolved`.

The file is uploaded to Discord's CDN so if you need it locally you should fetch the `.url` value, e.g. by using
something like dpp::cluster::request().

~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main()
{
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {

		/* Check which command they ran */
		if (event.command.get_command_name() == "show") {

			/* Get the file id from the parameter attachment. */
			dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("file"));

			/* Get the attachment that the user inputted from the file id. */
			dpp::attachment att = event.command.get_resolved_attachment(file_id);

			/* Reply with the file as a URL. */
			event.reply(att.url);
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {

			/* Create a new command. */
			dpp::slashcommand newcommand("show", "Show an uploaded file", bot.me.id);

			/* Add a parameter option. */
			newcommand.add_option(dpp::command_option(dpp::co_attachment, "file", "Select an image"));

			/* Register the command */
			bot.global_command_create(newcommand);
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~~~~~~~
