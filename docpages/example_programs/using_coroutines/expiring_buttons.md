\page expiring-buttons Making expiring buttons with when_any

\include{doc} coro_warn.dox

In the last example we've explored how to \ref awaiting-events "await events" using coroutines, we ran into the problem of the coroutine waiting forever if the button was never clicked. Wouldn't it be nice if we could add an "or" to our algorithm, for example wait for the button to be clicked *or* for a timer to expire? I'm glad you asked! D++ offers \ref dpp::when_any "when_any" which allows exactly that. It is a templated class that can take any number of awaitable objects and can be `co_await`-ed itself, will resume when the __first__ awaitable completes and return a \ref dpp::when_any::result "result" object that allows to retrieve which awaitable completed as well as its result, in a similar way as std::variant.

~~~~~~~~~~cpp
#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

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

			auto result = co_await dpp::when_any{ // Whichever completes first...
				event.from->creator->on_button_click.when([&id](const dpp::button_click_t &b) { return b.custom_id == id; }), // Button clicked
				event.from->creator->co_sleep(5) // Or sleep 5 seconds
			};
            // Note!! Due to a bug in g++11 and g++12, id must be captured as a reference above or the compiler will destroy it twice. This is fixed in g++13
			if (result.index() == 0) { // Awaitable #0 completed first, that is the button click event
                // Acknowledge the click with an empty message and edit the original response, removing the button
				auto &click_event = result.get<0>();
				click_event.reply(dpp::ir_deferred_update_message, dpp::message{});
				event.edit_original_response(dpp::message{"You clicked the button with the id " + click_event.custom_id});
			}
			else { // Here index() is 1, the timer expired
				event.edit_original_response(dpp::message{"I haven't got all day!"});
            }
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

Any awaitable can be used with when_any, even dpp::task, dpp::coroutine, dpp::async. When the when_any object is destroyed, any of its awaitables with a cancel() method (for example \ref dpp::task::cancel "dpp::task") will have it called. With this you can easily make commands that ask for input in several steps, or maybe a timed text game, the possibilities are endless! Note that if the first awaitable completes with an exception, result.get will throw it.

\note when_any will try to construct awaitable objects from the parameter you pass it, which it will own. In practice this means you can only pass it <a href="https://www.learncpp.com/cpp-tutorial/value-categories-lvalues-and-rvalues/">temporary objects (rvalues)</a> as most of the coroutine-related objects in D++ are move-only.
