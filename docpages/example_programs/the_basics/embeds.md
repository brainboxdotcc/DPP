\page embed-message Sending Embeds

If you've been in a server and used a bot or even seen a message from a bot, you might have seen a special message type, often sent by these bots. These are called embeds! In this section, we will show how to create an embed and reply to a user, using our newly created embed!

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {
    /* Setup the bot */
    dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

    /* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name == "embed") {

            /* Create an embed */
            dpp::embed embed = dpp::embed().
                set_color(dpp::colors::sti_blue).
                set_title("Some name").
                set_url("https://dpp.dev/").
                set_author("Some name", "https://dpp.dev/", "https://dpp.dev/DPP-Logo.png").
                set_description("Some description here").
                set_thumbnail("https://dpp.dev/DPP-Logo.png").
                add_field(
                        "Regular field title",
                        "Some value here"
                ).
                add_field(
                        "Inline field title",
                        "Some value here",
                        true
                ).
                add_field(
                        "Inline field title",
                        "Some value here",
                        true
                ).
                set_image("https://dpp.dev/DPP-Logo.png").
                set_footer(dpp::embed_footer().set_text("Some footer text here").set_icon("https://dpp.dev/DPP-Logo.png")).
                set_timestamp(time(0));

			/* Create a message with the content as our new embed. */
			dpp::message msg(event.command.channel_id, embed);

			/* Reply to the user with the message, containing our embed. */
			event.reply(msg);
		}
	});

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {

            /* Create and register a command when the bot is ready */
            bot.global_command_create(dpp::slashcommand("embed", "Send a test embed!", bot.me.id));
        }
    });

    bot.start(dpp::st_wait);
    return 0;
}
~~~~~~~~~~

The code will send the following message.

\image html embed.png
