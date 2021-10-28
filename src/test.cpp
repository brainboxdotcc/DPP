#undef DPP_BUILD
#ifdef _WIN32
_Pragma("warning( disable : 4251 )"); // 4251 warns when we export classes or structures with stl member variables
#endif
#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>
 
using json = nlohmann::json;

int main()
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;
	dpp::cluster bot(configdocument["token"]);

	/* The interaction create event is fired when someone issues your commands */
	bot.on_interaction_create([&bot](const dpp::interaction_create_t & event) {
		if (event.command.type == dpp::it_application_command) {
			dpp::command_interaction cmd_data = std::get<dpp::command_interaction>(event.command.data);
			/* Check which command they ran */
			if (cmd_data.name == "blep") {
				/* Fetch a parameter value from the command parameters */
				std::string animal = std::get<std::string>(event.get_parameter("animal"));
				/* Reply to the command. There is an overloaded version of this
				* call that accepts a dpp::message so you can send embeds.
				*/
				event.reply(dpp::ir_channel_message_with_source, "Blep! You chose " + animal);
			}
		}
	});
 
	bot.on_ready([&bot](const dpp::ready_t & event) {
 
		dpp::slashcommand newcommand;
		/* Create a new global command on ready event */
		newcommand.set_name("blep")
			.set_description("Send a random adorable animal photo")
			.set_application_id(bot.me.id)
			.add_option(dpp::command_option(dpp::co_string, "animal", "The type of animal").set_auto_complete(true)
		);

		std::cout << newcommand.build_json() << "\n";
 
		/* Register the command */
		bot.guild_command_create(newcommand, 825407338755653642, [&bot](const dpp::confirmation_callback_t &callback) {
			if (callback.is_error()) {
				bot.log(dpp::ll_error, "Failed to register slash command: " + callback.http_info.body);
			} else {
				bot.log(dpp::ll_debug, "Slash registered: " + callback.http_info.body);
			}
		});
	});

	bot.on_autocomplete([&bot](const dpp::autocomplete_t & event) {
		for (auto & opt : event.options) {
			if (opt.focused) {
				bot.interaction_response_create(event.command.id, event.command.token, dpp::interaction_response(dpp::ir_autocomplete_reply)
					.add_autocomplete_choice(dpp::command_option_choice("squids", "lots of squids"))
					.add_autocomplete_choice(dpp::command_option_choice("cats", "a few cats"))
					.add_autocomplete_choice(dpp::command_option_choice("dogs", "bucket of dogs"))
					.add_autocomplete_choice(dpp::command_option_choice("elephants", "bottle of elephants"))
				,
				[&bot](const dpp::confirmation_callback_t &callback) {
					if (callback.is_error()) {
						bot.log(dpp::ll_error, "Failed to respond: " + callback.http_info.body);
					} else {
						bot.log(dpp::ll_debug, "Responded: " + callback.http_info.body);
					}
				});
				std::cout << "Autocomplete " << opt.name << " with value of '" << opt.value << "' in field " << event.name << "\n";
				break;
			}
		}
	});


	bot.on_log([&bot](const dpp::log_t & event) {
		std::cout << dpp::utility::loglevel(event.severity) << ": " << event.message << "\n";
	});

	bot.start(false);

	return 0;
}