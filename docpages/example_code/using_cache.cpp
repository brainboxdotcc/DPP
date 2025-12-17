#include <dpp/dpp.h>

int main() {
	/* Create bot */
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_guild_members);

	bot.on_log(dpp::utility::cout_logger());

	/* This event is fired when someone removes their reaction from a message */
	bot.on_message_reaction_remove([&bot](const dpp::message_reaction_remove_t& event) {
		/* Find the user in the cache using his discord id */
		dpp::user* reacting_user = dpp::find_user(event.reacting_user_id);

		/* If user not found in cache, log and return */
		if (!reacting_user) {
			bot.log(dpp::ll_info, "User with the id " + std::to_string(event.reacting_user_id) + " was not found.");
			return;
		}

		bot.log(dpp::ll_info, reacting_user->format_username() + " removed his reaction.");
	});

	bot.start(dpp::st_wait);

	return 0;
}
