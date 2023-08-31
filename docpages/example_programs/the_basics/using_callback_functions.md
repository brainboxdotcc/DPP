\page callback-functions Using Callback Functions

When you create or get an object from Discord, you send the request to its API and in return you get either an error or the object you requested/created. Let's create a message and get it:

~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <fmt/format.h> // used to format text, install and define FMT_HEADER_ONLY to use

struct GetMessage {
    dpp::snowflake message_id = 1146725845457719367ULL;
    dpp::snowflake channel_id = 1112789762521182211ULL;
};

int main() {
    GetMessage gm;

    std::string BOT_TOKEN = "Token Was here";

    /* the second argument is a bitmask of intents - i_message_content is needed to get messages */
    dpp::cluster bot(BOT_TOKEN, dpp::i_message_content | dpp::i_default_intents);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&bot, gm](const dpp::slashcommand_t& event) -> void {
        if (event.command.get_command_name() == "msg-get") {

            // get a message using its own and channel's ID
            bot.message_get(gm.message_id, gm.channel_id, [&bot, event](const dpp::confirmation_callback_t& callback) -> void {
                if (callback.is_error()) { // catching an error to log it
                    bot.log(dpp::loglevel::ll_error, callback.get_error().message);
                    return;
                }

                // callback.value will contain the message we are getting but the type will be std::variant, so we will have to get the message itself.
                auto message = callback.get <dpp::message>();
                //std::get <dpp::message>(callback.value) would give the same result

                event.reply(message); // replies with the same message
            });
        }
        if (event.command.get_command_name() == "channel-create") {

            //create a text channel
            dpp::channel channel = dpp::channel()
                .set_name("test")
                .set_type(dpp::channel_type::CHANNEL_TEXT)
                .set_guild_id(1112789761032200306)
                .set_parent_id(0);
            bot.channel_create(channel, [&bot, event](const dpp::confirmation_callback_t& callback) -> void {
                if (callback.is_error()) { // catching an error to log it
                    bot.log(dpp::loglevel::ll_error, callback.get_error().message);
                }

                // callback.value will contain the channel we are creating but the type will be std::variant, so we will have to get the channel itself.
                auto channel = callback.get <dpp::channel>();
                // std::get <dpp::channel>(callback.value) would give the same result

                // reply with the created channel information
                dpp::message message = dpp::message(fmt::format("The channel's name is {0}, ID is {1} and type is {2}", channel.name, channel.id, channel.get_type()));
                /* note that channel types are represented as numbers */
                event.reply(message);
            });
        }
        if (event.command.get_command_name() == "msg-error") {
            bot.message_get(69, 420, [event](const dpp::confirmation_callback_t& callback) -> void {

                // the error will occur since there is no message with ID '69' that is in a channel with ID '420' (at least the bot can't see it)
                if (callback.is_error()) {
                    event.reply(callback.get_error().message);
                    return;
                }

                // we won't be able to get here because of the return; statement
                auto message = callback.get <dpp::message>();
                event.reply(message);
            });
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once <struct register_global_commands>()) {
            dpp::slashcommand msg_get("msg-get", "Get the message", bot.me.id);
            dpp::slashcommand channel_create("channel-create", "Create a channel", bot.me.id);
            dpp::slashcommand msg_error("msg-error", "Get an error instead of message :)", bot.me.id);

            bot.global_bulk_command_create({msg_get, channel_create, msg_error});
        }
    });

    bot.start(dpp::st_wait);
    return 0;
}
~~~~~~~~~~~~~~

This is the result:

\image html callback_functions.png