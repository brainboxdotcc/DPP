\page embed-message Sending Embeds

You might have seen these special messages, often sent by bots. In this section, we will show how to create an embed.

@note Because this example utilizes message content, it requires the message content privileged intent. 

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {
    /* Setup the bot */
    dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

    /* Message handler to look for a command called !embed */
    bot.on_message_create([&bot](const dpp::message_create_t & event) {
        if (event.msg.content == "!embed") {

            /* create the embed */
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

            /* reply with the created embed */
            bot.message_create(dpp::message(event.msg.channel_id, embed).set_reference(event.msg.id));
        }
    });

    bot.start(dpp::st_wait);
    return 0;
}
~~~~~~~~~~

The code will send the following message.

\image html embed.png
