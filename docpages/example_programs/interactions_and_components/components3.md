\page components3 Using select menu components

This example demonstrates creating a select menu, receiving select menu clicks and sending a response message.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {

	dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		
		/* Check which command they ran */
		if (event.command.get_command_name() == "select") {

			/* Create a message */
			dpp::message msg(event.command.channel_id, "This text has a select menu!");
			
			/* Add an action row, and a select menu within the action row. */
			msg.add_component(
				dpp::component().add_component(
					dpp::component().
					set_type(dpp::cot_selectmenu).
					set_placeholder("Pick something").
					add_select_option(dpp::select_option("label1","value1","description1").set_emoji(u8"😄")).
					add_select_option(dpp::select_option("label2","value2","description2").set_emoji(u8"🙂")).
					set_id("myselectid")
				)
			);

			/* Reply to the user with our message. */
			event.reply(msg);
		}
	});
	
	/* When a user clicks your select menu , the on_select_click event will fire,
	 * containing the custom_id you defined in your select menu.
	 */
	bot.on_select_click([&bot](const dpp::select_click_t & event) {
		/* Select clicks are still interactions, and must be replied to in some form to
		 * prevent the "this interaction has failed" message from Discord to the user.
		 */
		event.reply("You clicked " + event.custom_id + " and chose: " + event.values[0]);
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {

            /* Create and register a command when the bot is ready */
            bot.global_command_create(dpp::slashcommand("select", "Select something at random!", bot.me.id));
        }
    });

	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
