\page attach-file Attaching a file to a message

Attached files must be locally stored.

To attach a file to a message, you can upload a local image.

D++ has this helper function to read a file: dpp::utility::read_file.

An example program:

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {
    dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());
    
    /* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name == "file") {

            dpp::message msg(event.msg.channel_id, "Hey there, I've got a new file!");

            /* attach the file to the message */
            msg.add_file("foobar.txt", dpp::utility::read_file("path_to_your_file.txt"));

			/* Reply to the user with the message, with our file attached. */
			event.reply(msg);
		}
	});

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {

            /* Create and register a command when the bot is ready */
            bot.global_command_create(dpp::slashcommand("file", "Send a message with a file attached!", bot.me.id));
        }
    });

    bot.start(dpp::st_wait);

    return 0;
}
~~~~~~~~~~

Attachments via an url aren't possible. But there's a workaround for. You can download the file and then attach it to the message.

To make requests, D++ also has a helper function: dpp::cluster::request.

The following example program shows how to request a file and attach it to a message.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {
    dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

    /* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name == "file") {

            /* Request the image from the URL specified and capture the event in a lambda. */
            bot.request("https://dpp.dev/DPP-Logo.png", dpp::m_get, [event](const dpp::http_request_completion_t & httpRequestCompletion) {

                /* Create a message */
                dpp::message msg(event.command.channel_id, "This is my new attachment:");

                /* Attach the image to the message, only on success (Code 200). */
                if (httpRequestCompletion.status == 200) {
                    msg.add_file("logo.png", httpRequestCompletion.body);
                }

                /* Send the message, with our attachment. */
                event.reply(msg);
            });
		}
	});

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {

            /* Create and register a command when the bot is ready */
            bot.global_command_create(dpp::slashcommand("file", "Send a message with an image attached from the internet!", bot.me.id));
        }
    });

    bot.start(dpp::st_wait);

    return 0;
}
~~~~~~~~~~

Here's another example of how to add a local image to an embed.

Upload the image in the same message as the embed and then reference it in the embed.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {
    dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

    /* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name == "file") {

            /* Create a message. */
            dpp::message msg(event.msg.channel_id, "");

            /* Attach the image to the message we just created. */
            msg.add_file("image.jpg", dpp::utility::read_file("path_to_your_image.jpg"));

            /* Create an embed. */
            dpp::embed embed;
            embed.set_image("attachment://image.jpg"); /* Set the image of the embed to the attached image. */

            /* Add the embed to the message. */
            msg.add_embed(embed);

            event.reply(msg);
		}
	});

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {

            /* Create and register a command when the bot is ready */
            bot.global_command_create(dpp::slashcommand("file", "Send a local image along with an embed with the image!", bot.me.id));
        }
    });

    bot.start(dpp::st_wait);

    return 0;
}
~~~~~~~~~~
