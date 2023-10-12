#include <dpp/dpp.h>

int main() {
	/* Create the bot */
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const dpp::ready_t& event) {
		/* We don't need the run_once here as we're not registering commands! */

		/* Set the bot presence as online and "Playing..."! */
		bot.set_presence(dpp::presence(dpp::ps_online, dpp::at_game, "games!"));
	});

	bot.start(dpp::st_wait);

	return 0;
}
