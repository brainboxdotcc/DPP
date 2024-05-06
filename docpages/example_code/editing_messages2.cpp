#include <dpp/dpp.h>

int main() {
	/* the second argument is a bitmask of intents - i_message_content is needed to get messages */
	dpp::cluster bot("Token", dpp::i_default_intents | dpp::i_message_content);

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		if (event.command.get_command_name() == "channel-edit") {
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
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {

		if (dpp::run_once <struct register_global_commands>()) {
			dpp::slashcommand channel_edit("channel-edit", "Edit the name of channel specified", bot.me.id);

			channel_edit.add_option(dpp::command_option(dpp::co_channel, "channel", "Channel to edit", true));
			channel_edit.add_option(dpp::command_option(dpp::co_string, "name", "New name for the channel", true));

			bot.global_command_create(channel_edit);
		}
	});

	bot.start(dpp::st_wait);
	return 0;
}
