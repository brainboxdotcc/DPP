#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		/* Check which command they ran */
		if (event.command.get_command_name() == "addrole") {
			/* Fetch a parameter value from the command options */
			dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("user"));
			dpp::snowflake role_id = std::get<dpp::snowflake>(event.get_parameter("role"));

			/* Get member object from resolved list */
			dpp::guild_member resolved_member = event.command.get_resolved_member(user_id);

			resolved_member.add_role(role_id);
			bot.guild_edit_member(resolved_member);

			event.reply("Added role");
		}
	});

	/* Attach on_ready event */
	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			dpp::slashcommand add_role("addrole", "Give user a role", bot.me.id);

			/* Add user and role type command options to the slash command */
			add_role.add_option(dpp::command_option(dpp::co_user, "user", "User to give role to", true));
			add_role.add_option(dpp::command_option(dpp::co_role, "role", "Role to give", true));

			bot.global_command_create(add_role);
		}
	});

	/* Start bot */
	bot.start(dpp::st_wait);

	return 0;
}
