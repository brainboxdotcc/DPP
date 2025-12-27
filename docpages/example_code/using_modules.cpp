/* If import std; is supported, use it instead */
#include <cstdlib>

import dpp;

using dpp::cluster;
using dpp::slashcommand;
using dpp::start_type;

int main() {
	cluster bot(std::getenv("BOT_TOKEN"));

	bot.on_slashcommand([](auto event) {
		if (event.command.get_command_name() == "ping") {
			event.reply("Pong!");
		}
	});

	bot.on_ready([&bot](auto event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			bot.global_command_create(
				slashcommand("ping", "Ping pong!", bot.me.id)
			);
		}
	});

	bot.start(start_type::st_wait);
	return 0;
}