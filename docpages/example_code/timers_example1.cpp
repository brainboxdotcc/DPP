#include <dpp/dpp.h>

int main() {
	/* Create the bot */
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const dpp::ready_t& event) {
		/* Create a timer when the bot starts. */
		bot.start_timer([&bot](const dpp::timer& timer){
			/* Create a timer when the bot starts. */
			bot.request("https://dpp.dev/DPP-Logo.png", dpp::m_get, [&bot](const dpp::http_request_completion_t& callback) {
				/* Create a message to our desired channel, with the D++ logo. */
				bot.message_create(dpp::message(1140010849432522843, "").add_file("image.png", callback.body));
			});
		}, 10); /* Do it every 10 seconds. Timers also start with this delay. */
	});

	bot.start(dpp::st_wait);
}
