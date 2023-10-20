#include <dpp/dpp.h>

int main() {
	dpp::cluster bot{"token"};

	bot.on_log(dpp::utility::cout_logger());

	bot.on_slashcommand([](const dpp::slashcommand_t& event) -> dpp::task<void> {
		if (event.command.get_command_name() == "test") {
			// Make a message and add a button with its custom ID set to the command interaction's ID so we can identify it
			dpp::message m{"Test"};
			std::string id{event.command.id.str()};
			m.add_component(
				dpp::component{}.add_component(
					dpp::component{}
						.set_type(dpp::cot_button)
						.set_label("Click me!")
						.set_id(id)
				)
			);
			co_await event.co_reply(m);

			dpp::button_click_t click_event = co_await event.from->creator->on_button_click.when(
				// Note!! Due to a bug in g++11 and g++12, id must be captured as a reference here or the compiler will destroy it twice. This is fixed in g++13
				[&id] (dpp::button_click_t const &b) {
					return b.custom_id == id;
				}
			);
			// Acknowledge the click and edit the original response, removing the button
			click_event.reply();
			event.edit_original_response(dpp::message{"You clicked the button!"});
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			dpp::slashcommand command{"test", "Test awaiting for an event", bot.me.id};

			bot.global_command_create(command);
		}
	});

	bot.start(dpp::st_wait);

	return 0;
}
