#include <dpp/dpp.h>

int main() {
	/* Create the bot */
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const dpp::ready_t& event) {
		/* We put our status updating inside "run_once" so that multiple shards don't try do this as "set_presence" updates all the shards. */
		if (dpp::run_once<struct register_bot_commands>()) {
			/* We update the presence now as the timer will do the first execution after the x amount of seconds we specify */
			bot.set_presence(dpp::presence(dpp::presence_status::ps_online, dpp::activity_type::at_game, "with " + std::to_string(event.guild_count) + " guilds!"));

			/* Create a timer that runs every 120 seconds, that sets the status */
			bot.start_timer([&bot](const dpp::timer& timer) {
				bot.set_presence(dpp::presence(dpp::presence_status::ps_online, dpp::activity_type::at_game, "with " + std::to_string(dpp::get_guild_cache()->count()) + " guilds!"));
			}, 120);
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
