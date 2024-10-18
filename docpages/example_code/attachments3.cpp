#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "file") {
			/* Create a message. */
			dpp::message msg(event.command.channel_id, "");

			/* Attach the image to the message we just created. */
			msg.add_file("image.jpg", dpp::utility::read_file("path_to_your_image.jpg"));

			/* Create an embed. */
			dpp::embed embed;
			embed.set_image("attachment://image.jpg"); /* Set the image of the embed to the attached image. */

			/* Add the embed to the message. */
			msg.add_embed(embed);

			event.reply(msg);
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create and register a command when the bot is ready */
			bot.global_command_create(dpp::slashcommand("file", "Send a local image along with an embed with the image!", bot.me.id));
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
