#include <dpp/dpp.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <dpp/message.h>

using json = nlohmann::json;

int main()
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;

	dpp::cluster bot(configdocument["token"], dpp::i_default_intents | dpp::i_guild_members, 1);

	bot.on_log([](const dpp::log_t & event) {
		if (event.severity > dpp::ll_trace) {
			std::cout << event.message << "\n";
		}
	});

	bot.on_button_click([&bot](const dpp::button_click_t & event) {
		event.reply(dpp::ir_channel_message_with_source, "Clicky McClickface");
	});

	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		if (event.msg->content == "!ping2") {
			bot.message_create(
				dpp::message(event.msg->channel_id, "this text has buttons").add_component(
					dpp::component().add_component(
						dpp::component().set_label("Click me!").
						set_emoji("😄").
						set_style(dpp::cos_danger).
						set_id("myid")
					)
				)
			);
		}
	});

	bot.start(false);

	return 0;
}
