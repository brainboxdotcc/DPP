#include <dpp/dpp.h>

int main() {
	dpp::cluster bot{"token"};

	bot.on_log(dpp::utility::cout_logger());

	bot.on_slashcommand([](const dpp::slashcommand_t& event) -> dpp::task<void> {
		if (event.command.get_command_name() == "addemoji") {
			dpp::cluster *cluster = event.from->creator;
			// Retrieve parameter values
			dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("file"));
			std::string emoji_name = std::get<std::string>(event.get_parameter("name"));

			// Get the attachment from the resolved list
			const dpp::attachment &attachment = event.command.get_resolved_attachment(file_id);

			// For simplicity for this example we only support PNG
			if (attachment.content_type != "image/png") {
				// While we could use event.co_reply, we can just use event.reply, as we will exit the command anyway and don't need to wait on the result
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

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			dpp::slashcommand command("addemoji", "Add an emoji", bot.me.id);
			// Add file and name as required parameters
			command.add_option(dpp::command_option(dpp::co_attachment, "file", "Select an image", true));
			command.add_option(dpp::command_option(dpp::co_string, "name", "Name of the emoji to add", true));

			bot.global_command_create(command);
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
