#include <dpp/dpp.h>
#include <iostream>
#include <format>

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

			/* Add a channel selection */
			modal.add_component(
				dpp::component()
                    .set_label("Select channel")
                        .set_id("field_id2")
                    .set_type(dpp::cot_channel_selectmenu)
			);
    

			/* Trigger the dialog box. All dialog boxes are ephemeral */
			event.dialog(modal);
		}
	});

	/* This event handles form submission for the modal dialog we create above */
	bot.on_form_submit([](const dpp::form_submit_t & event) {
		/* For this simple example, we know the elements value type is string.
		 * We also know the indices of each element.
		 * In the real world, it may not be safe to make such assumptions!
		 */
		std::string message_text = std::get<std::string>(event.components[0].value);
		std::string channel_id = std::get<std::string>(event.components[1].value);

		dpp::message m;
		m.set_content(
			std::string("You entered '") + message_text + 
			"', picked channel: <#" + channel_id + ">"
		)
			.set_flags(dpp::m_ephemeral);

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
