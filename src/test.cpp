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


	/* The interaction create event is fired when someone issues your commands */
	bot.on_interaction_create([&bot](const dpp::interaction_create_t & event) {
		if (event.command.type == dpp::it_application_command) {
			dpp::command_interaction cmd_data = std::get<dpp::command_interaction>(event.command.data);
			/* Check which command they ran */
			if (cmd_data.name == "testaction") {
				/* Fetch a parameter value from the command parameters */
				event.reply(dpp::ir_channel_message_with_source, "You chose this bot's application context menu action!");
			}
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		dpp::slashcommand newcommand;
		/* Create a new global command on ready event */
		newcommand.set_name("testaction")
			.set_description("Test context menu")
			.set_type(dpp::ctxm_user)
			.set_application_id(bot.me.id);
		/* Register the command */
		bot.guild_command_create(newcommand, 633212509242785792, [&bot](const dpp::confirmation_callback_t &callback) {
			std::cout << callback.http_info.body << "\n";
		});
	});

	bot.on_log([](const dpp::log_t & event) {
		if (event.severity > dpp::ll_trace) {
		}
	});

	bot.start(false);
	return 0;
}
