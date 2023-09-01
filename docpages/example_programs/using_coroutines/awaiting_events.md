\page awaiting-events Waiting for events

\include{doc} coro_warn.dox

D++ makes it possible to await events: simple use `co_await` on any of the event routers, such as \ref dpp::cluster::on_message_create "on_message_create", and your coroutine will be suspended until the next event fired by this event router. You can also `co_await` the return of an event router's \ref dpp::event_router_t::when "when()" method while passing it a predicate function object, it will only resume your coroutine when the predicate returns true. Be aware that your coroutine is attached to the event router only when you call `co_await` and not before, and will be detached as it is resumed.

\note When the event router resumes your coroutine, it will give you __a reference to the event object__. This will likely mean it will be destroyed after your next co_await, make sure to save it in a local variable if you need it for longer.

~~~~~~~~~~cpp
#include <dpp/dpp.h>

int main() {
	dpp::cluster bot{"token"};

	bot.on_log(dpp::utility::cout_logger());

	bot.on_slashcommand([](dpp::slashcommand_t event) -> dpp::job {
		if (event.command.get_command_name() == "test") {
			// Make a message and add a button with its custom ID set to the command interaction's ID so we can identify it
			dpp::message m{"Test"};
			std::string id{event.command.id.str()};
			m.add_component(
				dpp::component{}.add_component(dpp::component{}.set_type(dpp::cot_button).set_label("Click me!").set_id(id))
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

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			dpp::slashcommand command{"test", "Test awaiting for an event", bot.me.id};

			bot.global_command_create(command);
		}
	});

	bot.start(dpp::st_wait);
}
~~~~~~~~~~

Note that there is a problem with that! If the user never clicks your button, or if the message gets deleted, your coroutine will be stuck waiting... And waiting... Forever until your bot shuts down, occupying a space in memory. This is where the \ref expiring-buttons "next example" comes into play as a solution, with a button that expires with time.

\image html waiting_coroutine.jpg
