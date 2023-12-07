#include <dpp/dpp.h>
#include <dpp/unicode_emoji.h>

int main() {
	dpp::cluster bot("Epic Token");

	bot.on_log(dpp::utility::cout_logger());

	/* We'll be using two emojis: shocked guy and animated mad face. */
	dpp::emoji shocked("vahuyi", 1179366531856093214);
	dpp::emoji mad("mad", 1117795317052616704, dpp::e_animated); /* We need this third argument, which is an emoji flag. */

	bot.on_slashcommand([shocked, mad](const dpp::slashcommand_t& event) {
		if (event.command.get_command_name() == "send-emojis") {
			/* Here we send our very informative message: three epic emojis. */
			event.reply(dpp::unicode_emoji::nerd + shocked.get_mention() + mad.get_mention());
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			bot.global_command_create(dpp::slashcommand("send-emojis", "Send the emojis", bot.me.id));
		}
	});

	/* Start the bot! */
	bot.start(dpp::st_wait);
	return 0;
}
