\page attach-file Attaching a file to a message

Attached files must be locally stored.

To attach a file to a message, you can upload a local image.

D++ has this helper function to read a file: `dpp::utility::read_file`.

An example program:

@note Because these examples utilizes message content, they require the message content privileged intent.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {
    dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

    bot.on_log(dpp::utility::cout_logger());

    /* Message handler to look for a command called !file */
    bot.on_message_create([&bot](const dpp::message_create_t &event) {
        if (event.msg.content == "!file") {
            // create a message
            dpp::message msg(event.msg.channel_id, "Hey there, i've got a new file!");

            // attach the file to the message
            msg.add_file("foobar.txt", dpp::utility::read_file("path_to_your_file.txt"));

            // send the message
            bot.message_create(msg);
        }
    });

    bot.start(false);
    return 0;
}
~~~~~~~~~~

Attachments via an url aren't possible. But there's a workaround for. You can download the file and then attach it to the message.

To make requests, D++ also has a helper function: `dpp::cluster::request`.

The following example program shows how to request a file and attach it to a message.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {
    dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

    bot.on_log(dpp::utility::cout_logger());

    /* Message handler to look for a command called !file */
    bot.on_message_create([&bot](const dpp::message_create_t &event) {
        if (event.msg.content == "!file") {
            // request an image
            bot.request("https://dpp.dev/DPP-Logo.png", dpp::m_get, [&bot, channel_id = event.msg.channel_id](const dpp::http_request_completion_t & httpRequestCompletion) {

                // create a message
                dpp::message msg(channel_id, "This is my new attachment:");

                // attach the image on success
                if (httpRequestCompletion.status == 200) {
                    msg.add_file("logo.png", httpRequestCompletion.body);
                }

                // send the message
                bot.message_create(msg);
            });
        }
    });

    bot.start(false);
    return 0;
}
~~~~~~~~~~

Here's another example of how to add a local image to an embed.

Upload the image in the same message as the embed and then reference it in the embed.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {
    dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

    bot.on_log(dpp::utility::cout_logger());

    /* Message handler to look for a command called !file */
    bot.on_message_create([&bot](const dpp::message_create_t &event) {
        if (event.msg.content == "!file") {
            // create a message
            dpp::message msg(event.msg.channel_id, "");

            // attach the image to the message
            msg.add_file("image.jpg", dpp::utility::read_file("path_to_your_image.jpg"));

            dpp::embed embed;
            embed.set_image("attachment://image.jpg"); // reference to the attached file
            msg.add_embed(embed);

            // send the message
            bot.message_create(msg);
        }
    });

    bot.start(false);
    return 0;
}
~~~~~~~~~~
