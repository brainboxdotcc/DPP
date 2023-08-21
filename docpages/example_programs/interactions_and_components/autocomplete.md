\page application-command-autocomplete Slash command auto completion

Discord now supports sending auto completion lists for slash command choices. To use this feature you can use code such as the example below:

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
 
int main()
{
	dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const dpp::ready_t & event) {
	    if (dpp::run_once<struct register_bot_commands>()) {

		    /* Create a new global command once on ready event */
		    bot.global_command_create(dpp::slashcommand("blep", "Send a random adorable animal photo", bot.me.id)
		    	.add_option(
		    		/* If you set the auto complete setting on a command option, it will trigger the on_autocomplete
		    		 * event whenever discord needs to fill information for the choices. You cannot set any choices
		    		 * here if you set the auto complete value to true.
		    		 */
		    		dpp::command_option(dpp::co_string, "animal", "The type of animal").set_auto_complete(true)
		    	)
		    );
		}
	});

	/* The interaction create event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t & event) {
		
		/* Check which command they ran */
		if (event.command.get_command_name() == "blep") {
			/* Fetch a parameter value from the command parameters */
			std::string animal = std::get<std::string>(event.get_parameter("animal"));
			/* Reply to the command. There is an overloaded version of this
			* call that accepts a dpp::message so you can send embeds.
			*/
			event.reply("Blep! You chose " + animal);
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
				bot.log(dpp::ll_debug, "Autocomplete " + opt.name + " with value '" + uservalue + "' in field " + event.name);
				break;
			}
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~

