#include <dpp/dpp.h>
#include <dpp/unicode_emoji.h>

int main() {
    dpp::cluster bot("Epic Token", dpp::i_default_intents | dpp::i_message_content);
    /* The second argument is a bitmask of intents - i_message_content is needed to see the messages */

    bot.on_log(dpp::utility::cout_logger());

    /* We'll be using a shocked guy emoji */
    dpp::emoji shocked("vahuyi", 1179366531856093214);

    bot.on_message_create([&bot, shocked](const dpp::message_create_t& event) {
        if (event.msg.content == "WHAT?") {
            /* If some unknown content shocks the user */

            bot.message_add_reaction(event.msg.id, event.msg.channel_id, shocked.format());
            /* React to their message with a shocked guy */
        } else if (event.msg.content == "I'm hungry") {
            /* But if they're hungry */

            bot.message_add_reaction(event.msg.id, event.msg.channel_id, dpp::unicode_emoji::cut_of_meat);
            /* Let's send some meat to the message, so they don't starve. They will thank us later. */
        }
    });

    bot.on_slashcommand([shocked](const dpp::slashcommand_t& event) {
        if (event.command.get_command_name() == "send-emoji") {

            /* Here we send our very informative message: two emojis next to each other.
             * We also have to use mention instead of format. */
            event.reply(shocked.get_mention() + dpp::unicode_emoji::cut_of_meat);
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            dpp::slashcommand send("send-emoji", "Send the emoji", bot.me.id);
            bot.global_bulk_command_create({ send });
        }
    });

    /* Start the bot! */
    bot.start(dpp::st_wait);
    return 0;
}
