#undef DPP_BUILD
#include <dpp/dpp.h>
#include <dpp/fmt/format.h>
#include <dpp/nlohmann/json.hpp>
 
using json = nlohmann::json;

int main()
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;
	dpp::cluster bot(configdocument["token"]);

	bot.on_ready([&bot](const dpp::ready_t & event) {
		bot.message_create(
			dpp::message(847003255708188682, "A message.").add_component(
				dpp::component().add_component(
				dpp::component().set_label("A Button").
				set_type(dpp::cot_button).
				set_emoji("📦").
				set_style(dpp::cos_danger).
				set_id("myid")
				).add_component(
				dpp::component().set_label("Another Button").
				set_type(dpp::cot_button).
				set_style(dpp::cos_primary).
				set_id("234")
				)
			)
		);
	});

	bot.start(false);

	return 0;
}