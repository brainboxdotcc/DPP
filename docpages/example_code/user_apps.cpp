#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token");
	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const auto& event) {
		if (dpp::run_once<struct boot_t>()) {
			/**
			 * Create a slash command which has interaction context 'itc_private_channel'.
			 * This is a user-app command which can be executed anywhere and is added to the user's profile.
			 */
			bot.global_bulk_command_create({
			       dpp::slashcommand("userapp", "Test user app command", bot.me.id)
			       .set_interaction_contexts({dpp::itc_guild, dpp::itc_bot_dm, dpp::itc_private_channel})
		       });
		}
	});

	bot.register_command("userapp", [](const dpp::slashcommand_t& e) {
		/**
		 * Simple test output that shows the context of the command
		 */
		e.reply("This is the `/userapp` command." + std::string(
			e.command.is_user_app_interaction() ?
			" Executing as a user interaction owned by user: <@" + e.command.get_authorizing_integration_owner(dpp::ait_user_install).str() + ">" :
			" Executing as a guild interaction on guild id " + e.command.guild_id.str()
		));
	});

	bot.start(dpp::st_wait);
}
