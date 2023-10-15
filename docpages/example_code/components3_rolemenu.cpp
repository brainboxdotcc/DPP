#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "select") {
			/* Create a message */
			dpp::message msg(event.command.channel_id, "This text has a select menu!");
			
			/* Add an action row, and a select menu within the action row. 
			 *
			 * By default, max values is 1, meaning people can only pick 1 option.
			 * We're changing this to two, so people can select multiple options!
			 * We'll also set the min_values to 2 so people have to pick another value!
			 */
			msg.add_component(
				dpp::component().add_component(
					dpp::component()
						.set_type(dpp::cot_role_selectmenu)
						.set_min_values(2)
						.set_max_values(2)
						.set_id("myselectid")
				)
			);

			/* Reply to the user with our message. */
			event.reply(msg);
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create and register a command when the bot is ready */
			bot.global_command_create(dpp::slashcommand("select", "Select something at random!", bot.me.id));
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
