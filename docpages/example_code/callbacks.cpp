#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("Token Was Here", dpp::i_default_intents | dpp::i_message_content);
	/* the second argument is a bitmask of intents - i_message_content is needed to get messages */

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		if (event.command.get_command_name() == "msgs-get") {
			int64_t limit = std::get<int64_t>(event.get_parameter("quantity"));

			/* get messages using ID of the channel the command was issued in */
			bot.messages_get(event.command.channel_id, 0, 0, 0, limit, [event](const dpp::confirmation_callback_t& callback) {
				if (callback.is_error()) { /* catching an error to log it */
					std::cout << callback.get_error().message << std::endl;
					return;
				}

				auto messages = callback.get<dpp::message_map>();
				/* std::get<dpp::message_map>(callback.value) would give the same result */

				std::string contents;
				for (const auto& x : messages) { /* here we iterate through the dpp::message_map we got from callback... */
					contents += x.second.content + '\n'; /* ...where x.first is ID of the current message and x.second is the message itself. */
				}

				event.reply(contents); /* we will see all those messages we got, united as one! */
			});
		} else if (event.command.get_command_name() == "channel-create") {
			/* create a text channel */
			dpp::channel channel = dpp::channel()
				.set_name("test")
				.set_guild_id(event.command.guild_id);

			bot.channel_create(channel, [&bot, event](const dpp::confirmation_callback_t& callback) -> void {
				if (callback.is_error()) { /* catching an error to log it */
					bot.log(dpp::loglevel::ll_error, callback.get_error().message);
					return;
				}

				auto channel = callback.get<dpp::channel>();
				/* std::get<dpp::channel>(callback.value) would give the same result */

				/* reply with the created channel information */
				dpp::message message = dpp::message("The channel's name is `" + channel.name + "`, ID is `" + std::to_string(channel.id) + " and type is `" + std::to_string(channel.get_type()) + "`.");
				/* note that channel types are represented as numbers */
				event.reply(message);
			});
		} else if (event.command.get_command_name() == "msg-error") {
			bot.message_get(0, 0, [event](const dpp::confirmation_callback_t& callback) -> void {
				/* the error will occur since there is no message with ID '0' that is in a channel with ID '0' (I'm not explaining why) */
				if (callback.is_error()) {
					event.reply(callback.get_error().message);
					return;
				}

				/* we won't be able to get here because of the return; statement */
				auto message = callback.get<dpp::message>();
				event.reply(message);
			});
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once <struct register_global_commands>()) {
			dpp::slashcommand msgs_get("msgs-get", "Get messages", bot.me.id);

			constexpr int64_t min_val{1};
			constexpr int64_t max_val{100};

			msgs_get.add_option(
				dpp::command_option(dpp::co_integer, "quantity", "Quantity of messages to get. Max - 100.")
					.set_min_value(min_val)
					.set_max_value(max_val)
			);

			dpp::slashcommand channel_create("channel-create", "Create a channel", bot.me.id);
			dpp::slashcommand msg_error("msg-error", "Get an error instead of message :)", bot.me.id);

			bot.global_bulk_command_create({ msgs_get, channel_create, msg_error });
		}
	});

	bot.start(dpp::st_wait);
	
	return 0;
}
