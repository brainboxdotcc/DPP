#include <dpp/dpp.h>
#include <chrono>
#include <thread>

int main()
{
	/* If you just want to fire webhooks, you can instantiate a cluster with no token */
	dpp::cluster bot;

	/* Start the cluster in its own thread */
	bot.start(dpp::st_return);

	/* Construct a webhook object using the URL you got from Discord */
	dpp::webhook wh("https://discord.com/api/webhooks/833047646548133537/ntCHEYYIoHSLy_GOxPx6pmM0sUoLbP101ct-WI6F-S4beAV2vaIcl_Id5loAMyQwxqhE");

	/* Send a message with this webhook asynchronously */
	bot.execute_webhook(wh, dpp::message("Have a great time here :smile:"));

	/* Wait here for the webhook to complete, but we could do anything we need here */
	while (bot.active_requests() > 0) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	/* When we return here, the cluster will be terminated */
	return 0;
}
