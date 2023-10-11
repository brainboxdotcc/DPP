#include <dpp/dpp.h>

int main() {
	/* Create the bot, but with our intents so we can use messages. */
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when the bot detects a message in any server and any channel it has access to. */
	bot.on_message_create([&bot](const dpp::message_create_t& event) {
		/* See if the message contains the phrase we want to check for.
		 * If there's at least a single match, we reply and say it's not allowed.
		 */
		if (event.msg.content.find("bad word") != std::string::npos) {
			event.reply("That is not allowed here. Please, mind your language!", true);
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}