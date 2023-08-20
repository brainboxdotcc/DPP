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
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content); /* Because we're handling messages, we need to use the "i_message_content" intent! */

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
	});

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "get") {

			dpp::message* find_msg = message_cache.find(std::get<std::string>(event.get_parameter("message_id")));

			/* If find_msg is null, tell the user and return. */
			if (!find_msg) {
				event.reply("There is no message cached with this ID");
				return;
			}

			event.reply("This message had the following content: " + find_msg->content);
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {

			/* Create a new command. */
			dpp::slashcommand newcommand("get", "Get the contents of a message that was cached via an id", bot.me.id);

			/* Add a parameter option. */
			newcommand.add_option(dpp::command_option(dpp::co_string, "message_id", "The ID of the message you want to find", true));

            /* Register the command */
            bot.global_command_create(newcommand);
        }
    });

	/* Start bot */
	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~

