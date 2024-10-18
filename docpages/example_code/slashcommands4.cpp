#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t & event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "ping") {
			event.reply("Pong!");
		} else if (event.command.get_command_name() == "pong") {
			event.reply("Ping!");
		} else if (event.command.get_command_name() == "ding") {
			event.reply("Dong!");
		} else if (event.command.get_command_name() == "dong") {
			event.reply("Ding!");
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create some commands */
			dpp::slashcommand pingcommand("ping", "Pong!", bot.me.id);
			dpp::slashcommand pongcommand("pong", "Ping!", bot.me.id);
			dpp::slashcommand dingcommand("ding", "Dong!", bot.me.id);
			dpp::slashcommand dongcommand("dong", "Ding!", bot.me.id);

			/* Register our commands in a list using bulk */
			bot.guild_bulk_command_create({ pingcommand, pongcommand, dingcommand, dongcommand }, 857692897221033129);
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
