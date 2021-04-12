#include <dpp/dpp.h>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <iomanip>

using json = nlohmann::json;

int main(int argc, char const *argv[])
{
	std::cout << "guild: " << sizeof(dpp::guild) << "\n";
	std::cout << "guild_member: " << sizeof(dpp::guild_member) << "\n";
	std::cout << "user: " << sizeof(dpp::user) << "\n";
	std::cout << "channel: " << sizeof(dpp::channel) << "\n";
	std::cout << "role: " << sizeof(dpp::role) << "\n";
	std::cout << "emoji: " << sizeof(dpp::emoji) << "\n";

	/* Read config file */
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;

	/* Create a D++ cluster. There are many more optional parameters but as this is a demo lets keep it simple. */
	dpp::cluster bot(configdocument["token"], dpp::i_default_intents | dpp::i_guild_members);

	/* Attach to the log event to log messages from the library.
	 * You can attach much fancier loggers to this such as spdlog,
	 * but that's up to you what you use.
	 */
	bot.on_log([&bot](const dpp::log_t & event) {
		/* Print all messages of level 'debug' or higher */
		if (event.severity >= dpp::ll_debug) {
			std::cout << dpp::utility::current_date_time() << " [" << dpp::utility::loglevel(event.severity) << "] " << event.message << "\n";
		}
	});

	/* Attach to the message_create event to get notified of new messages */
	bot.on_message_create([&bot](const dpp::message_create_t & event) {

		/* event.msg->content contains the message text */
		std::string content = event.msg->content;

		uint64_t member_count = bot.get_shards().begin()->second->GetMemberCount();

		/* Log some stats of the guild, user, role and channel counts, and the message content */
		bot.log(dpp::ll_info, fmt::format("[G:{} U:{} R:{} C:{} M:{}] <{}#{:04d}> {}", dpp::get_guild_count(), dpp::get_user_count(), dpp::get_role_count(), dpp::get_channel_count(), member_count, event.msg->author->username, event.msg->author->discriminator, content));

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
			bot.message_create(reply,[&bot](const dpp::confirmation_callback_t& completion) {
				/* This is called when the request is completed. 
				* completion.value contains the message object of the created message.
				* completion.type contains the string "message"
				* completion.http_info contains various information about the HTTP request which
				* was sent when issuing the command.
				*/
				dpp::message newmessage = std::get<dpp::message>(completion.value);
				bot.log(dpp::ll_debug, fmt::format("message created! id={} http status={}", newmessage.id, completion.http_info.status));
			});
		}
	});

	/* This method call actually starts the bot by connecting all shards in the cluster */
	bot.start();
	
	/* cluster::start() returns once all clusters are started. Clusters run in their own threads.
	 * Loop forever. We could do anything else we wanted here.
	 * DO NOT just do `while(true);` as this will eat CPU.
	 */
	while (true) {
		bot.set_presence(dpp::presence(dpp::ps_online, dpp::at_listening, "the screams of tortured souls"));
		
		std::this_thread::sleep_for(std::chrono::seconds(20));
	}
	
	/* Never reached */
	return 0;
}

