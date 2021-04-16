#include <dpp/dpp.h>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

int main(int argc, char const *argv[])
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;

	dpp::cluster bot(configdocument["token"], dpp::i_default_intents | dpp::i_guild_members, 1);

	bot.on_log([&bot](const dpp::log_t & event) {
		if (event.severity >= dpp::ll_debug) {
			std::cout << dpp::utility::current_date_time() << " [" << dpp::utility::loglevel(event.severity) << "] " << event.message << "\n";
		}
	});

	bot.on_interaction_create([&bot](const dpp::interaction_create_t & event) {
		bot.log(dpp::ll_debug, fmt::format("command id {} data.id {} name {}", event.command.id, event.command.data.id, event.command.data.name));
		if (event.command.data.name == "blep") {
			dpp::interaction_response ir;
			dpp::message m(event.command.channel_id, "Blep!");
			ir.type = dpp::ir_channel_message;
			*(ir.msg) = m;
			bot.interaction_response_create(event.command.id, event.command.token, ir, [&bot](const dpp::confirmation_callback_t & state) {
				bot.log(dpp::ll_debug, fmt::format("Interaction Response Result: {} -> {}", state.http_info.status, state.http_info.body));
			});
		}
	});

	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		std::stringstream ss(event.msg->content);
		std::string command;
		ss >> command;

		if (command == ".createslash") {
			dpp::slashcommand newcommand;
			newcommand.set_name("blep").set_description("Send a random adorable animal photo").set_application_id(bot.me.id);
			newcommand.add_option(
				dpp::command_option(dpp::co_string, "animal", "The type of animal", true).
					add_choice(dpp::command_option_choice("Dog", std::string("animal_dog"))).
					add_choice(dpp::command_option_choice("Cat", std::string("animal_cat"))).
					add_choice(dpp::command_option_choice("Penguin", std::string("animal_penguin"))
				)
			).add_option(
				dpp::command_option(dpp::co_boolean, "only_smol", "Whether to show only baby animals")
			);
			std::cout << newcommand.build_json(false) << "\n";
			bot.guild_command_create(newcommand, event.msg->guild_id, [&bot](const dpp::confirmation_callback_t & state) {
				bot.log(dpp::ll_debug, fmt::format("Application command tried. Result: {} -> {}", state.http_info.status, state.http_info.body));
			});
		}
	});

	/* This method call actually starts the bot by connecting all shards in the cluster */
	bot.start(false);
	
	/* Never reached */
	return 0;
}

