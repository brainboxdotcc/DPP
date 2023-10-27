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

			auto result = co_await dpp::when_any{ // Whichever completes first...
				event.from->creator->on_button_click.when([&id](const dpp::button_click_t &b) { // Button clicked
					return b.custom_id == id;
				}),
				event.from->creator->co_sleep(5) // Or sleep 5 seconds
			};
			// Note!! Due to a bug in g++11 and g++12, id must be captured as a reference above or the compiler will destroy it twice. This is fixed in g++13
			if (result.index() == 0) { // Awaitable #0 completed first, that is the button click event
				// Acknowledge the click and edit the original response, removing the button
				const dpp::button_click_t &click_event = result.get<0>();
				click_event.reply();
				event.edit_original_response(dpp::message{"You clicked the button with the id " + click_event.custom_id});
			} else { // Here index() is 1, the timer expired
				event.edit_original_response(dpp::message{"I haven't got all day!"});
			}
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
