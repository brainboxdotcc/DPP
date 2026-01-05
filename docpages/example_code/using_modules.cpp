import dpp;

using dpp::cluster;
using std::ready_t;
using dpp::slashcommand;
using dpp::slashcommand_t;
using dpp::start_type;

int main() {
	cluster bot("YOUR_BOT_TOKEN_HERE");

	bot.on_slashcommand([](const slashcommand_t& event) -> void {
		if (event.command.get_command_name() == "ping") {
			event.reply("Pong!");
		}
	});

	bot.on_ready([&bot](const ready_t& event) -> void {
		if (dpp::run_once<struct register_bot_commands>()) {
			bot.global_command_create(
				slashcommand("ping", "Ping pong!", bot.me.id)
			);
		}
	});

	bot.start(start_type::st_wait);
	return 0;
}
