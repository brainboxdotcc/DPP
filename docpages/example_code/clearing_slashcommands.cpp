#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* We won't be performing any commands, so we don't need to add the event here! */

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct clear_bot_commands>()) {
			/* Now, we're going to wipe our commands */
			bot.global_bulk_command_delete();
			/* This one requires a guild id, otherwise it won't know what guild's commands it needs to wipe! */
			bot.guild_bulk_command_delete(857692897221033129);
		}

		/* Because the run_once above uses a 'clear_bot_commands' struct, you can continue to register commands below! */
	});

	bot.start(dpp::st_wait);

	return 0;
}
