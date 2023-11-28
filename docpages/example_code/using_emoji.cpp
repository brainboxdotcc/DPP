#include <dpp/dpp.h>

int main() {
    dpp::cluster bot("Epic token", dpp::i_default_intents | dpp::i_message_content);

    bot.on_log(dpp::utility::cout_logger());

    dpp::emoji shocked("vahuyi", 1113825191978614837);
    dpp::emoji meat("cut_of_meat");

    bot.on_message_create([&bot, shocked, meat](const dpp::message_create_t& event) {
        if (event.msg.content == "WHAT?") {
            bot.message_add_reaction(event.msg.id, event.msg.channel_id, shocked.get_mention());
        } else if (event.msg.content == "I'm hungry") {
            bot.message_add_reaction(event.msg.id, event.msg.channel_id, meat.get_mention());
        }
    });

    bot.on_slashcommand([shocked, meat](const dpp::slashcommand_t& event) {
        if (event.command.get_command_name() == "send") {
            event.reply(shocked.get_mention() + meat.get_mention());
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            dpp::slashcommand send("send-emoji", "Send the emoji", bot.me.id);
            bot.global_bulk_command_create({ send });
        }
    });

    bot.start(dpp::st_wait);
    return 0;
}
