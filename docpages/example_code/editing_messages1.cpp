#include <dpp/dpp.h>

int main() {
	/* the second argument is a bitmask of intents - i_message_content is needed to get messages */
	dpp::cluster bot("Token", dpp::i_default_intents | dpp::i_message_content);

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		if (event.command.get_command_name() == "msg-send") {
			event.reply("That's a message");
		} else if (event.command.get_command_name() == "msg-edit") {
			const auto content = std::get<std::string>(event.get_parameter("content"));

			/* get the message to edit it after. here string will automatically be converted to snowflake */
			const dpp::snowflake msg_id = std::get<std::string>(event.get_parameter("msg-id"));

			bot.message_get(msg_id, event.command.channel_id, [&bot, content, event](const dpp::confirmation_callback_t& callback) {
				if (callback.is_error()) {
					event.reply("error");
					return;
				}
				auto message = callback.get<dpp::message>();

				/* change the message content and edit the message itself */
				message.set_content(content);
				bot.message_edit(message);
				event.reply("Message content is now `" + content + "`.");
			});
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once <struct register_global_commands>()) {
			dpp::slashcommand msg_edit("msg-edit", "Edit a message sent by the bot", bot.me.id);

			msg_edit.add_option(dpp::command_option(dpp::co_string, "msg-id", "ID of the message to edit", true)); /* true for required option */
			msg_edit.add_option(dpp::command_option(dpp::co_string, "content", "New content for the message", true)); /* same here */

			dpp::slashcommand msg_send("msg-send", "Send my message", bot.me.id);

			bot.global_bulk_command_create({ msg_edit, msg_send });
		}
	});

	bot.start(dpp::st_wait);
	return 0;
}
