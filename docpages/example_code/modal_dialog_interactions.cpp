#include <dpp/dpp.h>
#include <iostream>

int main() {
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check for our /dialog command */
		if (event.command.get_command_name() == "dialog") {
			/* Instantiate an interaction_modal_response object */
			dpp::interaction_modal_response modal("my_modal", "Please enter stuff");

			/* Add a text component */
			modal.add_component(
				dpp::component()
					.set_label("Short type rammel")
					.set_id("field_id")
					.set_type(dpp::cot_text)
					.set_placeholder("gumd")
					.set_min_length(5)
					.set_max_length(50)
					.set_text_style(dpp::text_short)
			);

			/* Add another text component in the next row, as required by Discord */
			modal.add_row();
			modal.add_component(
				dpp::component()
					.set_label("Type rammel")
					.set_id("field_id2")
					.set_type(dpp::cot_text)
					.set_placeholder("gumf")
					.set_min_length(1)
					.set_max_length(2000)
					.set_text_style(dpp::text_paragraph)
			);

			/* Trigger the dialog box. All dialog boxes are ephemeral */
			event.dialog(modal);
		}
	});

	/* This event handles form submission for the modal dialog we create above */
	bot.on_form_submit([](const dpp::form_submit_t & event) {
		/* For this simple example, we know the first element of the first row ([0][0]) is value type string.
		 * In the real world, it may not be safe to make such assumptions!
		 */
		std::string v = std::get<std::string>(event.components[0].components[0].value);

		dpp::message m;
		m.set_content("You entered: " + v).set_flags(dpp::m_ephemeral);

		/* Emit a reply. Form submission is still an interaction and must generate some form of reply! */
		event.reply(m);
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create a slash command and register it as a global command */
			bot.global_command_create(dpp::slashcommand("dialog", "Make a modal dialog box", bot.me.id));
		}
	});

	/* Start bot */
	bot.start(dpp::st_wait);

	return 0;
}
