\page components Using button components

Discord's newest features support sending buttons alongside messages, which when clicked by the user trigger an interaction which is routed by
D++ as an on_button_click event. To make use of this, use code as in this example.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>
#include <dpp/message.h>

int main() {

	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

	bot.on_log(dpp::utility::cout_logger());

	/* Message handler to look for a command called !button */
	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		if (event.msg.content == "!button") {
			/* Create a message containing an action row, and a button within the action row. */
			bot.message_create(
				dpp::message(event.msg.channel_id, "this text has buttons").add_component(
					dpp::component().add_component(
						dpp::component().set_label("Click me!").
						set_type(dpp::cot_button).
						set_emoji(u8"😄").
						set_style(dpp::cos_danger).
						set_id("myid")
					)
				)
			);
		}
	});

	/* When a user clicks your button, the on_button_click event will fire,
	 * containing the custom_id you defined in your button.
	 */
	bot.on_button_click([&bot](const dpp::button_click_t & event) {
		/* Button clicks are still interactions, and must be replied to in some form to
		 * prevent the "this interaction has failed" message from Discord to the user.
 		 */
		event.reply("You clicked: " + event.custom_id);
	});

	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the feature is functioning, the code below will produce buttons on the reply message like in the image below:

\image html button.png

