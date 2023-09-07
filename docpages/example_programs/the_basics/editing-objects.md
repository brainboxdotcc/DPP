\page editing-objects Editing Objects (like messages)

Sometimes we need to update an object, such as message or channel. At first, it might seem confusing, but it's actually really simple! It's done with [callbacks](https://dpp.dev/callback-functions.html). You just need to use an object with identical properties you don't need to update.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {
    dpp::cluster bot("Token is right here", dpp::i_default_intents | dpp::i_message_content);
    /* the second argument is a bitmask of intents - i_message_content is needed to get messages */

    bot.on_log(dpp::utility::cout_logger());

    /* The event is fired when someone issues your commands */
    bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) -> void {
        if (event.command.get_command_name() == "msg-send") {
            event.reply("That's a message");
        } else if (event.command.get_command_name() == "msg-edit") {
            std::string content = std::get<std::string>(event.get_parameter("content"));

            /* get message to edit it after */
            dpp::snowflake msg_id = std::get<std::string>(event.get_parameter("msg-id"));
            /* here string will automatically be converted to snowflake */

            bot.message_get(msg_id, event.command.channel_id, [&bot, content, event](const dpp::confirmation_callback_t& callback) -> void {
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
            auto name = std::get<std::string>(event.get_parameter("name"));

            /* get the channel to edit it after */
            auto channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));
            bot.channel_get(channel_id, [&bot, name, event](const dpp::confirmation_callback_t& callback) -> void {
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

    bot.on_ready([&bot](const dpp::ready_t& event) -> void {
        if (dpp::run_once <struct register_global_commands>()) {
            dpp::slashcommand msg_edit("msg-edit", "edit-msg", bot.me.id);

            msg_edit.add_option(
                dpp::command_option(dpp::co_string, "msg-id", "id of msg", true) /* true for required option */
            ).add_option(
                dpp::command_option(dpp::co_string, "content", "new content for msg", true) /* same here */
            );

            dpp::slashcommand channel_edit("channel-edit", "edit a channel", bot.me.id);

            channel_edit.add_option(
                dpp::command_option(dpp::co_channel, "channel", "channel to edit", true)
            ).add_option(
                dpp::command_option(dpp::co_string, "name", "new name for channel", true)
            );

            dpp::slashcommand msg_send("msg-send", "send-msg", bot.me.id);

            bot.global_bulk_command_create({ msg_edit, channel_edit, msg_send });
        }
    });
 
    bot.start(dpp::st_wait);
    return 0;
}
~~~~~~~~~~

Before editing:

\image html stuff_edit1.png

After editing:

\image html stuff_edit2.png