#include <dpp/dpp.h>

int main() {
	/* the second argument is a bitmask of intents - i_message_content is needed to get messages */
	dpp::cluster bot("Token", dpp::i_default_intents | dpp::i_message_content);

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		if (event.command.get_command_name() == "embed-send") {
			dpp::embed embed = dpp::embed()
				.set_color(dpp::colors::sti_blue)
				.set_title("like and subscribe")
				.set_url("https://dpp.dev/")
				.set_author("Some author", "https://dpp.dev/", "https://dpp.dev/DPP-Logo.png")
				.set_description("Creator is <creator name>");
			event.reply(embed);
		} else if (event.command.get_command_name() == "embed-edit") {
			const auto description = std::get<std::string>(event.get_parameter("desc"));

			/* get the message to edit its embed after. here string will automatically be converted to snowflake */
			const dpp::snowflake msg_id = std::get<std::string>(event.get_parameter("msg-id"));

			bot.message_get(msg_id, event.command.channel_id, [&bot, description, event](const dpp::confirmation_callback_t& callback) {
				if (callback.is_error()) {
					event.reply("error");
					return;
				}
				auto message = callback.get<dpp::message>();
				auto& embeds = message.embeds;

				/* change the embed description and edit the message itself.
				 * since we're using a reference, what changes in embeds changes in message.embeds
				 */
				embeds[0].set_description(description);

				bot.message_edit(message);
				event.reply("Embed description is now `" + description + "`.");
			});
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once <struct register_global_commands>()) {
			dpp::slashcommand embed_send("embed-send", "Send my embed", bot.me.id);

			dpp::slashcommand embed_edit("embed-edit", "Edit an embed sent by the bot", bot.me.id);

			embed_edit.add_option(dpp::command_option(dpp::co_string, "msg-id", "ID of the embed to edit", true)); /* true for required option */
			embed_edit.add_option(dpp::command_option(dpp::co_string, "desc", "New description for the embed", true)); /* same here */

			bot.global_bulk_command_create({ embed_send, embed_edit });
		}
	});

	bot.start(dpp::st_wait);
	return 0;
}
