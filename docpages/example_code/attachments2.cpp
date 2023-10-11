#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "file") {

			/* Request the image from the URL specified and capture the event in a lambda. */
			bot.request("https://dpp.dev/DPP-Logo.png", dpp::m_get, [event](const dpp::http_request_completion_t & httpRequestCompletion) {

				/* Create a message */
				dpp::message msg(event.command.channel_id, "This is my new attachment:");

				/* Attach the image to the message, only on success (Code 200). */
				if (httpRequestCompletion.status == 200) {
					msg.add_file("logo.png", httpRequestCompletion.body);
				}

				/* Send the message, with our attachment. */
				event.reply(msg);
			});
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create and register a command when the bot is ready */
			bot.global_command_create(dpp::slashcommand("file", "Send a message with an image attached from the internet!", bot.me.id));
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
