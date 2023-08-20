\page components2 Advanced button components

This example demonstrates adding multiple buttons, receiving button clicks and sending response messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {

	dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		
		/* Check which command they ran */
		if (event.command.get_command_name() == "math") {

			/* Create a message */
			dpp::message msg(event.command.channel_id, "What is 5+5?");
			
			/* Add an action row, and then 3 buttons within the action row. */
			msg.add_component(
				dpp::component().add_component(
					dpp::component().
					set_label("9").
					set_style(dpp::cos_primary).
					set_id("9")
				).add_component(
					dpp::component().
					set_label("10").
					set_style(dpp::cos_primary).
					set_id("10")
				).add_component(
					dpp::component().
					set_label("11").
					set_style(dpp::cos_primary).
					set_id("11")
				)
			);

			/* Reply to the user with our message. */
			event.reply(msg);
		}
	});

	bot.on_button_click([&bot](const dpp::button_click_t & event) {
		if (event.custom_id == "10") {
			event.reply(dpp::message("You got it right!").set_flags(dpp::m_ephemeral));
		} else {
			event.reply(dpp::message("Wrong! Try again.").set_flags(dpp::m_ephemeral));
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {

            /* Create and register a command when the bot is ready */
            bot.global_command_create(dpp::slashcommand("math", "A quick maths question!", bot.me.id));
        }
    });

	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This code will send a different message for correct and incorrect answers.

\image html button_2.png

