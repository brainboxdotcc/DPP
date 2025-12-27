\page making_a_http_request Making Arbitrary HTTP Requests Using D++

If you wish to make arbitrary HTTP(S) requests to websites and APIs, e.g. to update statistics on bot lists, you can use code similar to the code below. You may pass any arbitrary POST data:

```cpp
import std;
import dpp;

using dpp::cluster;
using dpp::slashcommand;
using dpp::start_type;

int main() {
	cluster bot(std::getenv("BOT_TOKEN"));

	bot.on_slashcommand([](auto event) {
		if (event.command.get_command_name() == "ping") {
			event.reply("Pong!");
		}
	});

	bot.on_ready([&bot](auto event) {
		if (dpp::run_once<struct RegisterBotCommands>()) {
			bot.global_command_create(
				slashcommand("ping", "Ping pong!", bot.me.id)
			);
		}
	});

	bot.start(start_type::st_wait);
	return 0;
}
```
