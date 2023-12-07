#include <dpp/dpp.h>
#include <dpp/unicode_emoji.h>

int main() {
	dpp::cluster bot("Epic Token", dpp::i_default_intents | dpp::i_message_content);
	/* The second argument is a bitmask of intents - i_message_content is needed to see the messages */

	bot.on_log(dpp::utility::cout_logger());

	/* We'll be using a shocked guy emoji */
	dpp::emoji shocked("vahuyi", 1179366531856093214);
	dpp::emoji mad("mad", 1117795317052616704, dpp::e_animated); /* We need this third argument, which is an emoji flag. */

	bot.on_message_create([&bot, shocked, mad](const dpp::message_create_t& event) {
		if (event.msg.content == "I'm hungry") {
			/* But if they're hungry */
			bot.message_add_reaction(event.msg.id, event.msg.channel_id, dpp::unicode_emoji::cut_of_meat);
			/* Let's send some meat to the message, so they don't starve. They will thank us later. */

		} else if (event.msg.content == "WHAT?") {
			/* If some unknown content shocked the user */
			bot.message_add_reaction(event.msg.id, event.msg.channel_id, shocked.format());
			/* React to their message with a shocked guy */

		} else if (event.msg.content == "I'm unsubscribing") {
			/* They are angry! We should also be! */
			bot.message_add_reaction(event.msg.id, event.msg.channel_id, mad.format());
			/* React to their message with a mad emoji */
		}
	});

	/* Start the bot! */
	bot.start(dpp::st_wait);
	return 0;
}
