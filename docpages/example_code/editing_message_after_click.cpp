#include <dpp/dpp.h>
#include <dpp/unicode_emoji.h>

int main() {
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "button") {
			/* Create a message */
			dpp::message msg(event.command.channel_id, "this text has a button");
			
			/* Add an action row, and then a button within the action row. */
			msg.add_component(
				dpp::component().add_component(
					dpp::component()
						.set_label("Click me!")
						.set_type(dpp::cot_button)
						.set_emoji(dpp::unicode_emoji::smile)
						.set_style(dpp::cos_danger)
						.set_id("myid")
				)
			);

			/* Reply to the user with our message. */
			event.reply(msg);
		}
	});

	/* When a user clicks your button, the on_button_click event will fire,
	 * containing the custom_id you defined in your button.
	 */
	bot.on_button_click([&bot](const dpp::button_click_t& event) {
		/* Instead of replying to the button click itself,
		 * we want to update the message that had the buttons on it.
		 */
		event.reply(dpp::ir_update_message, "You clicked: " + event.custom_id);
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create and register a command when the bot is ready */
			bot.global_command_create(dpp::slashcommand("button", "Send a message with a button!", bot.me.id));
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
