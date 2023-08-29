\page coro-simple-commands Making simple commands

\include{doc} coro_warn.dox

### Several steps in one

\note The next example assumes you are already familiar with how to use \ref firstbot "slash commands", \ref slashcommands "parameters", and \ref discord-application-command-file-upload "sending files through a command".

With coroutines, it becomes a lot easier to do several asynchronous requests for one task. As an example an "addemoji" command taking a file and a name as a parameter. This means downloading the emoji, submitting it to Discord, and finally replying, with some error handling along the way. Normally we would have to use callbacks and some sort of object keeping track of our state, but with coroutines, the function can simply pause and be resumed when we receive the response to our request :

~~~~~~~~~~cpp
#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

	bot.on_log(dpp::utility::cout_logger());

	bot.on_slashcommand([](dpp::slashcommand_t event) -> dpp::job {
		if (event.command.get_command_name() == "addemoji") {
			dpp::cluster *cluster = event.from->creator;
			// Retrieve parameter values
			dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("file"));
			std::string emoji_name = std::get<std::string>(event.get_parameter("name"));

			// Get the attachment from the resolved list
			const dpp::attachment &attachment = event.command.get_resolved_attachment(file_id);

			// For simplicity for this example we only support PNG
			if (attachment.content_type != "image/png") {
				// While event.co_reply is available, we can just use event.reply, as we will exit the command anyway and don't need to wait on the result
				event.reply("Error: type " + attachment.content_type + " not supported");
				co_return;
			}
			// Send a "<bot> is thinking..." message, to wait on later so we can edit
			dpp::async thinking = event.co_thinking(false);

			// Download and co_await the result
			dpp::http_request_completion_t response = co_await cluster->co_request(attachment.url, dpp::m_get);

			if (response.status != 200) { // Page didn't send the image
				co_await thinking; // Wait for the thinking response to arrive so we can edit
				event.edit_response("Error: could not download the attachment");
			} else {
				// Load the image data in a dpp::emoji
				dpp::emoji emoji(emoji_name);
				emoji.load_image(response.body, dpp::image_type::i_png);

				// Create the emoji and co_await the response
				dpp::confirmation_callback_t confirmation = co_await cluster->co_guild_emoji_create(event.command.guild_id, emoji);

				co_await thinking; // Wait for the thinking response to arrive so we can edit
				if (confirmation.is_error()) {
					event.edit_response("Error: could not add emoji: " + confirmation.get_error().message);
				} else { // Success
					event.edit_response("Successfully added " + confirmation.get<dpp::emoji>().get_mention()); // Show the new emoji
                }
			}
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			dpp::slashcommand command("addemoji", "Add an emoji", bot.me.id);

			// Add file and name as required parameters
			command.add_option(dpp::command_option(dpp::co_attachment, "file", "Select an image", true));
			command.add_option(dpp::command_option(dpp::co_string, "name", "Name of the emoji to add", true));

			bot.global_command_create(command);
		}
	});

	bot.start(dpp::st_wait);
}
~~~~~~~~~~

### I heard you liked tasks

\note This next example is fairly advanced and makes use of many of both C++ and D++'s advanced features.

Earlier we mentioned two other types of coroutines provided by dpp: dpp::coroutine and dpp::task. They both take their return type as a template parameter, which may be void. Both dpp::job and dpp::task start on the constructor for asynchronous execution, however only the latter can be `co_await`-ed, this allows you to retrieve its return value. If a dpp::task is destroyed before it ends, it is cancelled and will stop when it is resumed from the next `co_await`. dpp::coroutine also has a return value and can be `co_await`-ed, however it only starts when `co_await`-ing, meaning it is executed synchronously.

Here is an example of a command making use of dpp::task to retrieve the avatar of a specified user, or if missing, the sender:

~~~~~~~~~~cpp
#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

	bot.on_log(dpp::utility::cout_logger());

	bot.on_slashcommand([](dpp::slashcommand_t event) -> dpp::job {
		if (event.command.get_command_name() == "avatar") {
			// Make a nested coroutine to fetch the guild member requested, that returns it as an optional
			constexpr auto resolve_member = [](const dpp::slashcommand_t &event) -> dpp::task<std::optional<dpp::guild_member>> {
				const dpp::command_value &user_param = event.get_parameter("user");
				dpp::snowflake user_id;

				if (std::holds_alternative<std::monostate>(user_param))
					user_id = event.command.usr.id; // Parameter is empty so user is sender
				else if (std::holds_alternative<dpp::snowflake>(user_param))
					user_id = std::get<dpp::snowflake>(user_param); // Parameter has a user

				// If we have the guild member in the command's resolved data, return it
				const auto &member_map = event.command.resolved.members;
				if (auto member = member_map.find(user_id); member != member_map.end())
					co_return member->second;

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
				if (confirmation.is_error())
					co_return std::nullopt; // Member not found, return empty
				else
					co_return confirmation.get<dpp::guild_member>();
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


	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			dpp::slashcommand command("avatar", "Get your or another user's avatar image", bot.me.id);

			command.add_option(dpp::command_option(dpp::co_user, "user", "User to fetch the avatar from"));

			bot.global_command_create(command);
		}
	});

	bot.start(dpp::st_wait);
}
~~~~~~~~~~
