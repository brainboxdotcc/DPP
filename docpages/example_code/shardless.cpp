#include <dpp/dpp.h>

int main() {
	/* We must use the special constant NO_SHARDS here */
	dpp::cluster bot("TOKEN", 0, dpp::NO_SHARDS);

	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&](const dpp::ready_t& ready) {
		/* Enable discord interactions endpoint on port 3000
		 * NOTE: PUT YOUR OWN PUBLIC KEY HERE FOR THE FIRST PARAMETER.
		 * You can find this on the same page where you entered the Discord Interaction URL.
		 * Do not use your Discord bot token in this field!
		 */
		bot.enable_webhook_server("f8032a386dc1903787be887cd66d126e83eb3d481455aca509a4b8cbc526cafe", "0.0.0.0", 3000);
		/* Register a command */
		bot.global_command_create(dpp::slashcommand("hello", "Greets you", bot.me.id));

	});

	bot.on_slashcommand([&](const dpp::slashcommand_t& event) {
		event.reply("hello to you too");
	});

	bot.start(dpp::st_wait);
}
