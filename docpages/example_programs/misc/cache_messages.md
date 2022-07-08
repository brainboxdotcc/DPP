\page caching-messages Caching Messages

By default D++ does not cache messages. The example program below demonstrates how to instantiate a custom cache using dpp::cache which will allow you to cache messages and query the cache for messages by ID.

This can be adjusted to cache any type derived from dpp::managed including types you define yourself.

@note This example will cache and hold onto messages forever! In a real world situation this would be bad. If you do use this,
you should use the dpp::cache::remove() method periodically to remove stale items. This is left out of this example as a learning
exercise to the reader. For further reading please see the documentation of dpp::cache

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <sstream>

int main() {
	/* Create bot */
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

	/* Create a cache to contain types of dpp::message */
	dpp::cache<dpp::message> message_cache;

        bot.on_log(dpp::utility::cout_logger());

	/* Message handler */
	bot.on_message_create([&](const dpp::message_create_t &event) {

		/* Make a permanent pointer using new, for each message to be cached */
		dpp::message* m = new dpp::message();
		/* Store the message into the pointer by copying it */
		*m = event.msg;
		/* Store the new pointer to the cache using the store() method */
		message_cache.store(m);

		/* Simple ghetto command handler. In the real world, use slashcommand or commandhandler here. */
		std::stringstream ss(event.msg.content);
		std::string cmd;
		dpp::snowflake msg_id;
		ss >> cmd;

		/* Look for our command */
		if (cmd == "!get") {
			ss >> msg_id;
			/* Search our cache for a cached message */
			dpp::message* find_msg = message_cache.find(msg_id);
			if (find_msg != nullptr) {
				/* Found a cached message, echo it out */
				bot.message_create(dpp::message(event.msg.channel_id, "This message had the following content: " + find_msg->content));
			} else {
				/* Nothing like that here. Have you checked under the carpet? */
				bot.message_create(dpp::message(event.msg.channel_id, "There is no message cached with this ID"));
			}
		}
	});

	/* Start bot */
	bot.start(false);

	return 0;
}
~~~~~~~~~~

