#include <dpp/dpp.h>

std::map<dpp::snowflake, dpp::timer> user_timers{};

int main() {
	/* Create the bot */
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "start_timer") {
			/* Does user_timers contain the user id? */
			if (user_timers.find(event.command.usr.id) != user_timers.end()) {
				event.reply("You've already got an in-progress timer!");
				return;
			}

			/* Create a copy of the channel_id to copy in to the timer lambda. */
			dpp::snowflake channel_id = event.command.channel_id;

			/* Start the timer and save it to a local variable. */
			dpp::timer timer = bot.start_timer([&bot, channel_id](const dpp::timer& timer) {
				bot.message_create(dpp::message(channel_id, "This is a timed message! Use /stop_timer to stop this!"));
			}, 10);

			/*
			 * Add the timer to user_timers.
			 * As dpp::timer is just size_t (essentially the timer's ID), it's perfectly safe to copy it in.
			 */
			user_timers.emplace(event.command.usr.id, timer);

			event.reply("Started a timer every 10 seconds!");
		}

		if(event.command.get_command_name() == "stop_timer") {
			/* Is user_timers empty? */
			if (user_timers.empty()) {
				event.reply("There are no timers currently in-progress!");
				return;
			} else if (user_timers.find(event.command.usr.id) == user_timers.end()) { /* Does user_timers not contain the user id? */
				event.reply("You've don't currently have a timer in-progress!");
				return;
			}

			/* Stop the timer. */
			bot.stop_timer(user_timers[event.command.usr.id]);
			/* Remove the timer from user_timers. */
			user_timers.erase(event.command.usr.id);

			event.reply("Stopped your timer!");
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create a new global command on ready event. */
			dpp::slashcommand start_timer("start_timer", "Start a 10 second timer!", bot.me.id);
			dpp::slashcommand stop_timer("stop_timer", "Stop your 10 second timer!", bot.me.id);

			/* Register the commands. */
			bot.global_bulk_command_create({ start_timer, stop_timer });
		}
	});

	bot.start(dpp::st_wait);
}
