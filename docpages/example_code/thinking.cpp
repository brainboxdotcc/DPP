#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "thinking") {
			/*
			 * true for Ephemeral. 
			 * You can set this to false if you want everyone to see the thinking response.
			 */
			event.thinking(true, [event](const dpp::confirmation_callback_t& callback) {
				event.edit_original_response(dpp::message("thonk"));
			});

		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create a new global command on ready event */
			dpp::slashcommand newcommand("thinking", "Thinking example...", bot.me.id);

			/* Register the command */
			bot.global_command_create(newcommand);
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
