\page slashcommands Using Slash Commands and Interactions

Slash commands and interactions are a newer feature of Discord which allow bot's commands to be registered centrally within the system and for users to easily explore and get help with available commands through the client itself.

To add a slash command you should use the dpp::cluster::global_command_create method for global commands (available to all guilds) or dpp::cluster::guild_command_create to create a local command (available only to one guild).

When a user issues these commands the reply will arrive via the `on_slashcommand` event which you can hook, and take action when you see your commands. It is possible to reply to an interaction by using either the dpp::interaction_create_t::reply method, or by manually instantiating an object of type dpp::interaction_response and attaching a dpp::message object to it.

dpp::interaction_create_t::reply has two overloaded versions of the method, one of which accepts simple std::string replies, for basic text-only messages (if your message is 'ephemeral' you must use this) and one which accepts a dpp::message for more advanced replies. Please note that at present, Discord only supports a small subset of message and embed features within an interaction response object.

\note You can also use the unified command handler, which lets you combine channel based message commands and slash commands under the same lambda with the same code like they were one and the same. Note that after August of 2022 Discord will be discouraging bots from using commands that are prefixed messages via means of a privileged message intent. It is advised that you exclusively use slash commands, or the unified handler with only a prefix of "/" going forward for any new bots you create and look to migrating existing bots to this setup.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main()
{
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t & event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "blep") {
			/* Fetch a parameter value from the command parameters */
			std::string animal = std::get<std::string>(event.get_parameter("animal"));
			/* Reply to the command. There is an overloaded version of this
			* call that accepts a dpp::message so you can send embeds.
			*/
			event.reply(std::string("Blep! You chose") + animal);
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
	    if (dpp::run_once<struct register_bot_commands>()) {

            /* Create a new global command on ready event */
		    dpp::slashcommand newcommand("blep", "Send a random adorable animal photo", bot.me.id)
		    newcommand.add_option(
		    		dpp::command_option(dpp::co_string, "animal", "The type of animal", true).
		    			add_choice(dpp::command_option_choice("Dog", std::string("animal_dog"))).
		    			add_choice(dpp::command_option_choice("Cat", std::string("animal_cat"))).
		    			add_choice(dpp::command_option_choice("Penguin", std::string("animal_penguin")
		    		)
		    	)
		    );

		    /* Register the command */
		    bot.global_command_create(newcommand);
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

\note For demonstration purposes, and small bots, this code is OK, but in the real world once your bot gets big, it's not recommended to create slash commands in the `on_ready` event because it gets called often (discord forces reconnections and sometimes these do not resume). You could for example add a commandline parameter to your bot (`argc`, `argv`) so that if you want the bot to register commands it must be launched with a specific command line argument.

