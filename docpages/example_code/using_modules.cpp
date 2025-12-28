import dpp;

int main() {
	dpp::cluster bot("YOUR_BOT_TOKEN_HERE");

	bot.on_slashcommand([](const dpp::slashcommand_t& event) -> void {
		if (event.command.get_command_name() == "ping") {
			event.reply("Pong!");
		}
	});

	bot.on_ready([&bot](const std::ready_t& event) -> void {
		if (dpp::run_once<struct register_bot_commands>()) {
			bot.global_command_create(
				dpp::slashcommand("ping", "Ping pong!", bot.me.id)
			);
		}
	});

	bot.start(dpp::start_type::st_wait);
	return 0;
}
