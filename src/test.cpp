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

	/* Create command handler, and specify prefixes */
	dpp::commandhandler command_handler(&bot);
	/* Specifying a prefix of "/" tells the command handler it should also expect slash commands */
	command_handler.add_prefix(".").add_prefix("/");

	bot.on_ready([&command_handler](const dpp::ready_t &event) {

		dpp::discord_client* d = event.from;

	});

	
	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		if (std::size(event.msg->stickers) > 0) {
			std::cout << event.msg->stickers[0].name << std::endl;
		}
    });


	bot.on_log([](const dpp::log_t & event) {
		if (event.severity > dpp::ll_trace) {
		}
	});

	bot.start(false);
	return 0;
}
