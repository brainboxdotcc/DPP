#include <dpp/dpp.h>

const std::string BOT_TOKEN = "add your token here";

int main() {
	dpp::cluster bot(BOT_TOKEN);

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			bot.global_command_create(dpp::slashcommand("ping", "Ping pong!", bot.me.id));
		}
	});
}
