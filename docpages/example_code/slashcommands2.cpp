#include <dpp/dpp.h>

int main() {
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
			dpp::slashcommand newcommand("blep", "Send a random adorable animal photo", bot.me.id);
			newcommand.add_option(
				dpp::command_option(dpp::co_string, "animal", "The type of animal", true)
					.add_choice(dpp::command_option_choice("Dog", std::string("animal_dog")))
					.add_choice(dpp::command_option_choice("Cat", std::string("animal_cat")))
					.add_choice(dpp::command_option_choice("Penguin", std::string("animal_penguin")))
			);

			/* Register the command */
			bot.guild_command_create(newcommand, 857692897221033129); /* Replace this with the guild id you want */
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
