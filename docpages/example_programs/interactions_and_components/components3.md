\page components3 Using select menu components

This example demonstrates receiving select menu clicks and sending response messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

using json = nlohmann::json;

int main() {

	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

        bot.on_log(dpp::utility::cout_logger());

	/* Message handler to look for a command called !select */
	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		if (event.msg.content == "!select") {
			/* Create a message containing an action row, and a select menu within the action row. */
			dpp::message m(event.msg.channel_id, "this text has a select menu");
			m.add_component(
				dpp::component().add_component(
					dpp::component().set_type(dpp::cot_selectmenu).
					set_placeholder("Pick something").
					add_select_option(dpp::select_option("label1","value1","description1").set_emoji(u8"😄")).
					add_select_option(dpp::select_option("label2","value2","description2").set_emoji(u8"🙂")).
					set_id("myselid")
				)
			);
			bot.message_create(m);
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

	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
