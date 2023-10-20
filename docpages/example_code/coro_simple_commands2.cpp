#include <dpp/dpp.h>

int main() {
	dpp::cluster bot{"token"};

	bot.on_log(dpp::utility::cout_logger());

	bot.on_slashcommand([](const dpp::slashcommand_t& event) -> dpp::task<void> {
		if (event.command.get_command_name() == "avatar") {
			// Make a nested coroutine to fetch the guild member requested, that returns it as an optional
			constexpr auto resolve_member = [](const dpp::slashcommand_t &event) -> dpp::task<std::optional<dpp::guild_member>> {
				const dpp::command_value &user_param = event.get_parameter("user");
				dpp::snowflake user_id;
				if (std::holds_alternative<std::monostate>(user_param)) {
					user_id = event.command.usr.id; // Parameter is empty so user is sender
				}
				else if (std::holds_alternative<dpp::snowflake>(user_param)) {
					user_id = std::get<dpp::snowflake>(user_param); // Parameter has a user
				}

				// If we have the guild member in the command's resolved data, return it
				const auto &member_map = event.command.resolved.members;
				if (auto member = member_map.find(user_id); member != member_map.end()) {
					co_return member->second;
				}
				// Try looking in guild cache
				dpp::guild *guild = dpp::find_guild(event.command.guild_id);
				if (guild) {
					// Look in guild's member cache
					if (auto member = guild->members.find(user_id); member != guild->members.end()) {
						co_return member->second;
					}
				}
				// Finally if everything else failed, request API
				dpp::confirmation_callback_t confirmation = co_await event.from->creator->co_guild_get_member(event.command.guild_id, user_id);
				if (confirmation.is_error()) {
					co_return std::nullopt; // Member not found, return empty
				} else {
					co_return confirmation.get<dpp::guild_member>();
				}
			};

			// Send a "<bot> is thinking..." message, to wait on later so we can edit
			dpp::async thinking = event.co_thinking(false);
			// Call our coroutine defined above to retrieve the member requested
			std::optional<dpp::guild_member> member = co_await resolve_member(event);
			if (!member.has_value()) {
				// Wait for the thinking response to arrive to make sure we can edit
				co_await thinking;
				event.edit_original_response(dpp::message{"User not found in this server!"});
				co_return;
			}

			std::string avatar_url = member->get_avatar_url(512);
			if (avatar_url.empty()) { // Member does not have a custom avatar for this server, get their user avatar
				dpp::confirmation_callback_t confirmation = co_await event.from->creator->co_user_get_cached(member->user_id);
				if (confirmation.is_error()) {
					// Wait for the thinking response to arrive to make sure we can edit
					co_await thinking;
					event.edit_original_response(dpp::message{"User not found!"});
					co_return;
				}
				avatar_url = confirmation.get<dpp::user_identified>().get_avatar_url(512);
			}

			// Wait for the thinking response to arrive to make sure we can edit
			co_await thinking;
			event.edit_original_response(dpp::message{avatar_url});
		}
	});


	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			dpp::slashcommand command("avatar", "Get your or another user's avatar image", bot.me.id);
			command.add_option(dpp::command_option(dpp::co_user, "user", "User to fetch the avatar from"));

			bot.global_command_create(command);
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
