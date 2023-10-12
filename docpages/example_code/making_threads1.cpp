#include <dpp/dpp.h>

int main() {
	/* Create the bot */
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when the bot detects a message in any server and any channel it has access to. */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "create-thread") {
			/* Here we create a thread in the current channel. It will expire after 60 minutes of inactivity. We'll also allow other mods to join, and we won't add a slowdown timer. */
			bot.thread_create("Cool thread!", event.command.channel_id, 60, dpp::channel_type::CHANNEL_PUBLIC_THREAD, true, 0, [event](const dpp::confirmation_callback_t& callback) {
				if (callback.is_error()) {
					event.reply("Failed to create a thread!");
					return;
				}
				
				event.reply("Created a thread for you!");
			});
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create and register the command */
			bot.global_command_create(dpp::slashcommand("create-thread", "Create a thread!", bot.me.id));
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
