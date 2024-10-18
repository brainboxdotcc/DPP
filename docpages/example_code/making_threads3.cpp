#include <dpp/dpp.h>
	 
int main() {
	/* Create the bot */
	dpp::cluster bot("token");
	
	bot.on_log(dpp::utility::cout_logger());
	
	/* The event is fired when the bot detects a message in any server and any channel it has access to. */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "lock-thread") {
			/* Get this channel as a thread. */
			bot.thread_get(event.command.channel_id, [&bot, event](const dpp::confirmation_callback_t& callback) {
				if (callback.is_error()) {
					event.reply("I failed to get the thread!");
					return;
				}

				/* Get the thread from the callback. */
				auto thread = callback.get<dpp::thread>();

				/* Set the thread to locked. */
				thread.metadata.locked = true;
				
				/* Now we tell discord about our updates, meaning the thread will lock! */
				bot.thread_edit(thread, [event](const dpp::confirmation_callback_t& callback2) {
					if (callback2.is_error()) {
						event.reply("I failed to lock the thread!");
						return;
					}

					event.reply("I have locked the thread!");
				});
			});
		}
	});
	
	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create and register the command */
			bot.global_command_create(dpp::slashcommand("lock-thread", "Lock the thread that you run this command in!", bot.me.id));
		}
	});
	
	bot.start(dpp::st_wait);
	
	return 0;
}