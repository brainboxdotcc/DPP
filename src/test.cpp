#include <dpp/dpp.h>
#include <iostream>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt/format.h>

using json = nlohmann::json;

int main()
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;
	dpp::cluster bot(configdocument["token"], dpp::i_default_intents | dpp::i_guild_members);

	/* Message handler to look for a command called !button */
	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		if (event.msg->content == "!button") {
			/* Create a message containing an action row, and a button within the action row. */
			dpp::message m(event.msg->channel_id, "this text has buttons");
			m.add_component(
				dpp::component().add_component(
					dpp::component().set_type(dpp::cot_selectmenu).
					set_placeholder("Pick something").
					add_select_option(dpp::select_option("label1","value1","description1").set_emoji("😄")).
					add_select_option(dpp::select_option("label2","value2","description2").set_emoji("🙂")).
					set_id("myselid")
				)
			);
			bot.message_create(m);
		}
	});
	/* When a user clicks your button, the on_button_click event will fire,
	 * containing the custom_id you defined in your button.
	 */
	bot.on_select_click([&bot](const dpp::select_click_t & event) {
		/* Select clicks are still interactions, and must be replied to in some form to
		 * prevent the "this interaction has failed" message from Discord to the user.
		 */
		event.reply(dpp::ir_channel_message_with_source, "You clicked " + event.custom_id + " and chose: " + event.values[0]);
	});

	bot.on_log([](const dpp::log_t & event) {
		if (event.severity > dpp::ll_trace) {
			std::cout << event.message << "\n";
		}
	});

	bot.start(false);
	return 0;
}
