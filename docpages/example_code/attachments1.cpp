#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "file") {
			dpp::message msg(event.command.channel_id, "Hey there, I've got a new file!");

			/* attach the file to the message */
			msg.add_file("foobar.txt", dpp::utility::read_file("path_to_your_file.txt"));

			/* Reply to the user with the message, with our file attached. */
			event.reply(msg);
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create and register a command when the bot is ready */
			bot.global_command_create(dpp::slashcommand("file", "Send a message with a file attached!", bot.me.id));
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
