\page modal-dialog-interactions Modal Dialog Interactions

Modal dialog interactions are a new Discord API feature that allow you to have pop-up windows which prompt the user to input information. Once the user has filled in this information, your program will receive an `on_form_submit` event which will contain the data which was input. You must use a slash command interaction response to submit your modal form data to Discord, via the `on_slashcommand` event. From here calling the `dialog` method of the `interaction_create_t` event object will trigger the dialog to appear.

Each dialog box may have up to five rows of input fields. The example below demonstrates a simple setup with just one text input:

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>

int main(int argc, char const *argv[])
{
	dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check for our /dialog command */
		if (event.command.get_command_name() == "dialog") {

			/* Instantiate an interaction_modal_response object */
			dpp::interaction_modal_response modal("my_modal", "Please enter stuff");

			/* Add a text component */
			modal.add_component(
				dpp::component().
				set_label("Short type rammel").
				set_id("field_id").
				set_type(dpp::cot_text).
				set_placeholder("gumd").
				set_min_length(5).
				set_max_length(50).
				set_text_style(dpp::text_short)
			);

			/* Add another text component in the next row, as required by Discord */
			modal.add_row();
			modal.add_component(
				dpp::component().
				set_label("Type rammel").
				set_id("field_id2").
				set_type(dpp::cot_text).
				set_placeholder("gumf").
				set_min_length(1).
				set_max_length(2000).
				set_text_style(dpp::text_paragraph)
			);

			/* Trigger the dialog box. All dialog boxes are ephemeral */
			event.dialog(modal);
		}
	});

	/* This event handles form submission for the modal dialog we create above */
	bot.on_form_submit([&](const dpp::form_submit_t & event) {
		
		/* For this simple example we know the first element of the first row ([0][0]) is value type string.
		 * In the real world it may not be safe to make such assumptions!
		 */
		std::string v = std::get<std::string>(event.components[0].components[0].value);

		dpp::message m;
		m.set_content("You entered: " + v).set_flags(dpp::m_ephemeral);

		/* Emit a reply. Form submission is still an interaction and must generate some form of reply! */
		event.reply(m);
	});

	bot.on_ready([&](const dpp::ready_t & event) {
	    if (dpp::run_once<struct register_bot_commands>()) {
			/* Create a slash command and register it as a global command */
		    bot.global_command_create(dpp::slashcommand("dialog", "Make a modal dialog box", bot.me.id));
		}
	});

	/* Start bot */

	bot.start(dpp::st_wait);
	return 0;
}
~~~~~~~~~~

If you compile and run this program and wait for the global command to register, typing `/dialog` will present you with a dialog box like the one below:

\image html modal_dialog.png

