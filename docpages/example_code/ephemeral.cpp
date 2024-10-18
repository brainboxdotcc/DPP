#include <dpp/dpp.h>

int main() {
	/* Create the bot */
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t & event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "hello") {
			/* Reply to the user, but only let them see the response. */
			event.reply(dpp::message("Hello! How are you today?").set_flags(dpp::m_ephemeral));
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create and Register the command */
			bot.global_command_create(dpp::slashcommand("hello", "Hello there!", bot.me.id));
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
