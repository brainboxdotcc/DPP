#define SPDLOG_FMT_EXTERNAL
#include <spdlog/spdlog.h>
#include <dpp/dpp.h>
#include <fstream>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char const *argv[])
{
	/* Read config file */
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;

	/* Create a D++ cluster. The intents, shard counts, cluster counts and logger are all optional. */
	dpp::cluster bot(configdocument["token"]);

	std::shared_ptr<spdlog::logger> log = bot.default_logger("test.log");
	bot.set_logger(log.get());

	/* Attach to the message_create event to get notified of new messages */
	bot.on_message_create([log, &bot](const dpp::message_create_t & event) {

		std::string content = event.msg->content;

		/* Log some stats of the guild, user, role and channel counts, and the message content */
		log->info("[G:{} U:{} R:{} C:{}] <{}#{:04d}> {}", dpp::get_guild_count(), dpp::get_user_count(), dpp::get_role_count(), dpp::get_channel_count(), event.msg->author->username, event.msg->author->discriminator, content);

		/* Crappy command handler example */
		if (content == ".dotest" && (event.msg->guild_id == 825407338755653642 || event.msg->guild_id == 828433613343162459)) {

			/* Fill a message object for a reply */
			dpp::message reply;
			reply.channel_id = event.msg->channel_id;
			reply.content = "Do your own test lazybones";

			/* Upload a test file along with the message */
			reply.filename = "test.txt";
			reply.filecontent = "Nothing to see here, move along.";

			/* Make a nice pretty embed */
			reply.embeds.push_back(dpp::embed().
					set_title("This is a test").
					set_color(0xff00ff).
					set_description("It is not a drill. THIS is a drill.").
					set_image("https://hips.hearstapps.com/hmg-prod.s3.amazonaws.com/images/2020-03-10-use-a-drill-final-clean-00-01-57-10-still053-1584632505.jpg")
			);

			/* Send message! */
			bot.message_create(reply,[log, &bot](const dpp::confirmation_callback_t& completion) {
				/* This is called when the request is completed. 
				 * completion.value contains the message object of the created message.
				 * completion.type contains the string "message"
				 * completion.http_info contains various information about the HTTP request which
				 * was sent when issuing the command.
				 */
				dpp::message newmessage = std::get<dpp::message>(completion.value);
				log->debug("message created! id={} http status={}", newmessage.id, completion.http_info.status);

			});
		}
	});

	/* This method call actually starts the bot by connecting all shards in the cluster */
	bot.start();
	
	/* cluster::start() returns once all clusters are started. Clusters run in their own threads. */
	while (true) {
		/* Loop forever. We could do anything else we wanted here.
		 * DO NOT just do `while(true);` as this will eat CPU.
		 */
		std::this_thread::sleep_for(std::chrono::seconds(60));
	}
	
	/* Never reached */
	return 0;
}

