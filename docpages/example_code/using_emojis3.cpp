#include <dpp/dpp.h>
#include <dpp/unicode_emoji.h>

int main() {
	dpp::cluster bot("Epic Token");

	bot.on_log(dpp::utility::cout_logger());

	/* We now have a new character! That's for the select menu. */
	dpp::emoji walter("walter_black", 1179374919088361544);
	dpp::emoji mad("mad", 1117795317052616704, dpp::e_animated); /* We need this third argument, which is an emoji flag. */

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([walter, mad](const dpp::slashcommand_t& event) {
		if (event.command.get_command_name() == "select") {
			dpp::message msg(event.command.channel_id, "Now.");
			msg.add_component(
				dpp::component().add_component(
					dpp::component()
						.set_type(dpp::cot_selectmenu)
						.set_placeholder("Say my name.")
						.add_select_option(dpp::select_option("Do what?", "Yeah, you do.", "I don't have a damn clue what you're talking about.").set_emoji(dpp::unicode_emoji::thinking))
						.add_select_option(dpp::select_option("Heisenberg", "You're goddamn right!", "The one and only").set_emoji(walter.name, walter.id))
						.add_select_option(dpp::select_option("I'm unsubscribing", "Wait what", "Pure cruelty").set_emoji(mad.name, mad.id, mad.is_animated())) /* Since our mad emoji is animated, we should tell that to the function */
						.set_id("myselectid")
				)
			);
			event.reply(msg);
		}
	});

	bot.on_select_click([](const dpp::select_click_t& event) {
		event.reply(event.values[0]);
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			bot.global_command_create(dpp::slashcommand("select", "Send the select menu", bot.me.id));
		}
	});

	/* Start the bot! */
	bot.start(dpp::st_wait);
	return 0;
}
