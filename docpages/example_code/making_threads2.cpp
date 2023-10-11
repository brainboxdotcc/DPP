#include <dpp/dpp.h>

int main() {
	/* Create the bot */
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when the bot detects a message in any server and any channel it has access to. */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t & event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "message-thread") {
			/* Get all active threads in a guild. */
			bot.threads_get_active(event.command.guild_id, [&bot, event](const dpp::confirmation_callback_t& callback) {
				if (callback.is_error()) {
					event.reply("Failed to get threads!");
					return;
				}
				
				/* Get the list of active threads in the guild. */
				auto threads = callback.get<dpp::active_threads>();
				
				dpp::snowflake thread_id;
				
				/* Loop through the threads, getting each value in the map. Then we get the first value and then break off.
				* The reason we're getting only the first value is because, for this example, we'll just assume you've only got a single active thread (the one created by the bot)
				*/
				for (const auto& _thread : threads) {
					thread_id = _thread.first;
					break;
				}
				
				/* Send a message in the first thread we find. */
				bot.message_create(dpp::message(thread_id, "Hey, I'm first to message in a cool thread!"), [event](const dpp::confirmation_callback_t& callback2) {
					if (callback2.is_error()) {
						event.reply("Failed to send a message in a thread.");
						return;
					}
				
					event.reply("I've sent a message in the specified thread.");
				});
			});
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create and register the command */
			bot.global_command_create(dpp::slashcommand("message-thread", "Message a thread!", bot.me.id));
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
