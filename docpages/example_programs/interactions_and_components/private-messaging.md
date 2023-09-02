\page private-messaging Sending private messages

Sometimes it's simply not enough to ping someone in a server with a message, and we get that. That's why you can private message people! This tutorial will cover how to make a command that will either message the author of the command or message a specified user! This is done with a command option! So, let's get into it!

\note This tutorial makes use of callbacks. For more information about that, visit \ref callback-functions "Using Callback Functions".

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main()
{
    /* Create the bot */
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

    /* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t & event) {

		/* Check which command they ran */
		if (event.command.get_command_name() == "pm") {

			/* If there was no input, we want to only send a message to the command author. */
            if(event.get_parameter("user").index() == 0) {

                /* Send a message to the command author. */
                bot.direct_message_create(event.command.get_issuing_user().id, dpp::message("Here's a private message!"), [event](const dpp::confirmation_callback_t& callback){
                    if(callback.is_error()) {
                        event.reply(dpp::message("I couldn't send a message to you.").set_flags(dpp::m_ephemeral));
                        return;
                    }

                    event.reply(dpp::message("I've sent you a private message.").set_flags(dpp::m_ephemeral));
                });

                /* Because we've sent the message, let's stop the function now. */
                return;
            }

            /* Here, we can assume that there was a user inputted, so we can just retrieve that data! */

            /* Get the user specified by the command author. */
            auto user = std::get<dpp::snowflake>(event.get_parameter("user"));

            /* Send a message the user the author mentioned, instead of the command author. */
            bot.direct_message_create(user, dpp::message("Here's a private message!"), [event](const dpp::confirmation_callback_t& callback){
                if(callback.is_error()) {
                    event.reply(dpp::message("I couldn't send a message to that user. Please check that is a valid user!").set_flags(dpp::m_ephemeral));
                    return;
                }

                event.reply(dpp::message("I've sent a message to that user.").set_flags(dpp::m_ephemeral));
            });
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
	    if (dpp::run_once<struct register_bot_commands>()) {

            /* Register the command */
            dpp::slashcommand command("pm", "Send a private message.", bot.me.id);

            /* Add the option for a user mention that isn't required */
            command.add_option(dpp::command_option(dpp::co_mentionable, "user", "The user to message", false));

		    /* Register the command */
		    bot.global_command_create(command);
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~

That's it! Now, you should have something like this:

\image html privatemessageexample.png
\image html privatemessageexample2.png