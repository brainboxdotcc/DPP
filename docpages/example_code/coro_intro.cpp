#include <dpp/dpp.h>

int main() {
	dpp::cluster bot{"token"};

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([](const dpp::slashcommand_t& event) -> dpp::task<void> {
		if (event.command.get_command_name() == "file") {
			/* Request the image from the URL specified and co_await the response */
			dpp::http_request_completion_t result = co_await event.from->creator->co_request("https://dpp.dev/DPP-Logo.png", dpp::m_get);

			/* Create a message and attach the image on success */
			dpp::message msg(event.command.channel_id, "This is my new attachment:");
			if (result.status == 200) {
				msg.add_file("logo.png", result.body);
			}

			/* Send the message, with our attachment. */
			event.reply(msg);
		}
	});
 
	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create and register a command when the bot is ready */
			bot.global_command_create(dpp::slashcommand{"file", "Send a message with an image attached from the internet!", bot.me.id});
		}
	});

	bot.start(dpp::st_wait);
	
	return 0;
}
