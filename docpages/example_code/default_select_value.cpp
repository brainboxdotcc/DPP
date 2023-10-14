#include <dpp/dpp.h>
#include <dpp/unicode_emoji.h>
	
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
			 * Your default values are limited to max_values,
			 * meaning you can't add more default values than the allowed max values.
			 */
			msg.add_component(
				dpp::component().add_component(
					dpp::component()
						.set_type(dpp::cot_role_selectmenu)
						.set_min_values(2)
						.set_max_values(2)
						.add_default_value(dpp::snowflake{667756886443163648}, dpp::cdt_role)
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