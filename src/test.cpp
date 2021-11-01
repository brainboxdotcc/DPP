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

	bot.on_ready([&bot](const dpp::ready_t & event) {
		/* Create a new global command on ready event */
		bot.guild_command_create(dpp::slashcommand().set_name("blep")
			.set_description("Send a random adorable animal photo")
			.set_application_id(bot.me.id)
			.add_option(
				/* If you set the auto complete setting on a command option, it will trigger the on_auticomplete
				 * event whenever discord needs to fill information for the choices. You cannot set any choices
				 * here if you set the auto complete value to true.
				 */
				dpp::command_option(dpp::co_string, "animal", "The type of animal").set_auto_complete(true)
			),
			825407338755653642);
	});

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
 
	/* The on_autocomplete event is fired whenever discord needs information to fill in a command options's choices.
	 * You must reply with a REST event within 500ms, so make it snappy!
	 */
	bot.on_autocomplete([&bot](const dpp::autocomplete_t & event) {
		for (auto & opt : event.options) {
			/* The option which has focused set to true is the one the user is typing in */
			if (opt.focused) {
				/* In a real world usage of this function you should return values that loosely match
				 * opt.value, which contains what the user has typed so far. The opt.value is a variant
				 * and will contain the type identical to that of the slash command parameter.
				 * Here we can safely know it is string.
				 */
				std::string uservalue = std::get<std::string>(opt.value);
				bot.interaction_response_create(event.command.id, event.command.token, dpp::interaction_response(dpp::ir_autocomplete_reply)
					.add_autocomplete_choice(dpp::command_option_choice("squids", "lots of squids"))
					.add_autocomplete_choice(dpp::command_option_choice("cats", "a few cats"))
					.add_autocomplete_choice(dpp::command_option_choice("dogs", "bucket of dogs"))
					.add_autocomplete_choice(dpp::command_option_choice("elephants", "bottle of elephants"))
				);
				bot.log(dpp::ll_debug, "Autocomplete " + opt.name + " with value of '" + uservalue + "' in field " + event.name);
				break;
			}
		}
	});

	/* Simple log event */
	bot.on_log([&bot](const dpp::log_t & event) {
		std::cout << dpp::utility::loglevel(event.severity) << ": " << event.message << "\n";
	});

	bot.start(false);

	return 0;
}
