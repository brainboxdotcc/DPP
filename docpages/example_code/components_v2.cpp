#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const auto& event) {
		if (dpp::run_once<struct boot_t>()) {
			bot.global_bulk_command_create({ dpp::slashcommand("cats", "I love cats", bot.me.id) });
		}
	});

	bot.on_button_click([](const dpp::button_click_t& event) {
		event.reply("You declared your love for cats by clicking button id: " + event.custom_id);
	});

	/* This is a detailed example of using many different types of component. For a complete
	 * list of supported components, see the Discord developer documentation and the definition
	 * of dpp::component_type.
	 */
	bot.register_command("cats", [](const dpp::slashcommand_t& e) {
		e.reply(dpp::message()
			/* Remember to set the message flag for components v2 */
			.set_flags(dpp::m_using_components_v2).add_component_v2(
			/* Reply with a container... */
			dpp::component()
				.set_type(dpp::cot_container)
				.set_accent(dpp::utility::rgb(255, 0, 0))
				.set_spoiler(true)
				.add_component_v2(
					/* ...which contains a section... */
					dpp::component()
						.set_type(dpp::cot_section)
						.add_component_v2(
							/* ...with text... */
							dpp::component()
								.set_type(dpp::cot_text_display)
								.set_content("Click if you love cats")
						)
						.set_accessory(
							/* ...and an accessory button to the right */
							dpp::component()
								.set_type(dpp::cot_button)
								.set_label("Click me")
								.set_style(dpp::cos_danger)
								.set_id("button")
						)
				)
		).add_component_v2(
			/* ... with a large visible divider between... */
			dpp::component()
				.set_type(dpp::cot_separator)
				.set_spacing(dpp::sep_large)
				.set_divider(true)
		).add_component_v2(
			/* ... followed by a media gallery... */
			dpp::component()
				.set_type(dpp::cot_media_gallery)
				.add_media_gallery_item(
					/* ...containing one cat pic (obviously) */
					dpp::component()
						.set_type(dpp::cot_thumbnail)
						.set_description("A cat")
						.set_thumbnail("https://www.catster.com/wp-content/uploads/2023/11/Beluga-Cat-e1714190563227.webp")
				)
		));
	});

	bot.start(dpp::st_wait);
}
