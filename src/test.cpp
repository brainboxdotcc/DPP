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
	});

	bot.on_message_reaction_add([&bot](const dpp::message_reaction_add_t &event) {
		std::cout << "Reaction added to message: " << event.message_id << " the emoji name is:" << event.reacting_emoji.name << "\n";
	});

	bot.on_message_reaction_remove([&bot](const dpp::message_reaction_remove_t &event) {
		std::cout << "Reaction removed from message: " << event.message_id << " the emoji name is:" << event.reacting_emoji.name << "\n";
	});

	bot.start(false);

	return 0;
}