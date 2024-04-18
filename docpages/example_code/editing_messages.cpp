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
		} else if (event.command.get_command_name() == "channel-edit") {
			const auto name = std::get<std::string>(event.get_parameter("name"));

			/* get the channel to edit it after */
			const auto channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));
			bot.channel_get(channel_id, [&bot, name, event](const dpp::confirmation_callback_t& callback) {
				if (callback.is_error()) {
					event.reply("error");
					return;
				}
				auto channel = callback.get<dpp::channel>();

				/* change the channel name and edit the channel itself */
				channel.set_name(name);
				bot.channel_edit(channel);
				event.reply("Channel name is now `" + name + "`.");
			});
		} else if (event.command.get_command_name() == "embed-send") {
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
			dpp::slashcommand msg_edit("msg-edit", "Edit a message sent by the bot", bot.me.id);

			msg_edit.add_option(dpp::command_option(dpp::co_string, "msg-id", "ID of the message to edit", true)); /* true for required option */
			msg_edit.add_option(dpp::command_option(dpp::co_string, "content", "New content for the message", true)); /* same here */

			dpp::slashcommand channel_edit("channel-edit", "Edit the name of channel specified", bot.me.id);

			channel_edit.add_option(dpp::command_option(dpp::co_channel, "channel", "Channel to edit", true));
			channel_edit.add_option(dpp::command_option(dpp::co_string, "name", "New name for the channel", true));

			dpp::slashcommand msg_send("msg-send", "Send my message", bot.me.id);

			dpp::slashcommand embed_send("embed-send", "Send my embed", bot.me.id);

			dpp::slashcommand embed_edit("embed-edit", "Edit an embed sent by the bot", bot.me.id);

			embed_edit.add_option(dpp::command_option(dpp::co_string, "msg-id", "ID of the embed to edit", true)); /* true for required option */
			embed_edit.add_option(dpp::command_option(dpp::co_string, "desc", "New description for the embed", true)); /* same here */

			bot.global_bulk_command_create({ msg_edit, channel_edit, msg_send, embed_send, embed_edit });
		}
	});

	bot.start(dpp::st_wait);
	return 0;
}
