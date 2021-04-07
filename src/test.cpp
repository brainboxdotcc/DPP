#define SPDLOG_FMT_EXTERNAL
#include <dpp/dpp.h>
#include <fstream>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

using json = nlohmann::json;

int main(int argc, char const *argv[])
{
	/* Read config file */
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;

	/* An example of how to set up an spdlog logger like the one in aegis */
	std::shared_ptr<spdlog::logger> log;
	spdlog::init_thread_pool(8192, 2);
	std::vector<spdlog::sink_ptr> sinks;
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
	auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("test.txt", 1024 * 1024 * 5, 10);
	sinks.push_back(stdout_sink);
	sinks.push_back(rotating);
	log = std::make_shared<spdlog::async_logger>("test", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	spdlog::register_logger(log);
	log->set_pattern("%^%Y-%m-%d %H:%M:%S.%e [%L] [th#%t]%$ : %v");
	log->set_level(spdlog::level::level_enum::debug);

	/* Lets just output some log */
	log->info("Starting test bot");

	/* Create a D++ cluster. The intents, shard counts, cluster counts and logger are all optional. */
	dpp::cluster bot(configdocument["token"]);

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

