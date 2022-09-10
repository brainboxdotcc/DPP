\page components2 Advanced components

This example demonstrates receiving button clicks and sending response messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

using json = nlohmann::json;

int main() {

	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content); // Privileged intent required to receive message content

        bot.on_log(dpp::utility::cout_logger());

	bot.on_button_click([&bot](const dpp::button_click_t & event) {
		if (event.custom_id == "10") {
			event.reply(dpp::message("Correct").set_flags(dpp::m_ephemeral));
		} else {
			event.reply(dpp::message("Incorrect").set_flags(dpp::m_ephemeral));
		}
	});

	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		if (event.msg.content == "!ping2") {
			bot.message_create(
				dpp::message(event.msg.channel_id, "What is 5+5?").add_component(
					dpp::component().add_component(
						dpp::component().set_label("9").
						set_style(dpp::cos_primary).
						set_id("9")
					).add_component(
						dpp::component().set_label("10").
						set_style(dpp::cos_primary).
						set_id("10")
					).add_component(
						dpp::component().set_label("11").
						set_style(dpp::cos_primary).
						set_id("11")
					)
				)
			);
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This code will send a different message for correct and incorrect answers.

\image html button_2.png

